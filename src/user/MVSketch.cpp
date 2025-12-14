#include "MVSketch.h"

MVSketchUser::MVSketchUser()
    : skel_(nullptr),
      select_counter_fd_(-1),
      select_lock_fd_(-1),
      current_active_(0),
      ifindex_(0) {
    counters_fd_[0] = -1;
    counters_fd_[1] = -1;
    counters_mmap_[0] = nullptr;
    counters_mmap_[1] = nullptr;
    lock_fd_[0] = -1;
    lock_fd_[1] = -1;

    skel_ = MVSketch::open_and_load();
    if (!skel_) {
        exit(-1);
    }

    // 初始化数据
    select_counter_fd_ = bpf_map__fd(skel_->maps.select_counter);
    if (select_counter_fd_ < 0) {
        MVSketch::destroy(skel_);
        exit(-1);
    }

    counters_fd_[0] = bpf_map__fd(skel_->maps.buckets_0);
    if (counters_fd_[0] < 0) {
        MVSketch::destroy(skel_);
        exit(-1);
    }
    counters_fd_[1] = bpf_map__fd(skel_->maps.buckets_1);
    if (counters_fd_[1] < 0) {
        MVSketch::destroy(skel_);
        exit(-1);
    }

    // 初始化锁
    select_lock_fd_ = bpf_map__fd(skel_->maps.select_lock);
    if (select_lock_fd_ < 0) {
        MVSketch::destroy(skel_);
        exit(-1);
    }

    lock_fd_[0] = bpf_map__fd(skel_->maps.locks_0);
    if (lock_fd_[0] < 0) {
        MVSketch::destroy(skel_);
        exit(-1);
    }
    lock_fd_[1] = bpf_map__fd(skel_->maps.locks_1);
    if (lock_fd_[1] < 0) {
        MVSketch::destroy(skel_);
        exit(-1);
    }

    // mmap 数据
    size_t mmap_size = MV_ROWS * MV_COLS * MMAP_STRIDE(MVBucket);
    counters_mmap_[0] =
        static_cast<MVBucket*>(mmap(nullptr, mmap_size, PROT_READ | PROT_WRITE,
                                    MAP_SHARED, counters_fd_[0], 0));
    if (counters_mmap_[0] == MAP_FAILED) {
        MVSketch::destroy(skel_);
        exit(-1);
    }
    counters_mmap_[1] =
        static_cast<MVBucket*>(mmap(nullptr, mmap_size, PROT_READ | PROT_WRITE,
                                    MAP_SHARED, counters_fd_[1], 0));
    if (counters_mmap_[1] == MAP_FAILED) {
        munmap(counters_mmap_[0], mmap_size);
        MVSketch::destroy(skel_);
        exit(-1);
    }

    current_active_ = 0;
}

MVSketchUser::~MVSketchUser() {
    detach();

    // 取消 mmap 映射
    size_t mmap_size = MV_ROWS * MV_COLS * MMAP_STRIDE(MVBucket);
    if (counters_mmap_[0] != nullptr && counters_mmap_[0] != MAP_FAILED) {
        munmap(counters_mmap_[0], mmap_size);
    }
    if (counters_mmap_[1] != nullptr && counters_mmap_[1] != MAP_FAILED) {
        munmap(counters_mmap_[1], mmap_size);
    }

    if (skel_) {
        MVSketch::destroy(skel_);
        skel_ = nullptr;
    }
}

int MVSketchUser::attach(const char* ifname) {
    if (!ifname) {
        return -EINVAL;
    }

    // 解析网卡名
    unsigned int ifindex = if_nametoindex(ifname);
    if (ifindex == 0) {
        return -ENODEV;
    }

    // 拿到 update 函数描述符
    int fd = bpf_program__fd(skel_->progs.update);
    if (fd < 0) {
        return fd;
    }

    // 把描述符插入到 XDP 上
    int ret = bpf_xdp_attach(ifindex, fd, XDP_FLAGS_UPDATE_IF_NOEXIST, nullptr);
    if (ret < 0) {
        return ret;
    }

    ifindex_ = ifindex;
    return 0;
}

void MVSketchUser::detach() {
    if (ifindex_ == 0) {
        return;
    }
    bpf_xdp_detach(ifindex_, XDP_FLAGS_UPDATE_IF_NOEXIST, nullptr);
    ifindex_ = 0;
}

int MVSketchUser::swap() {
    int new_active = 1 - current_active_;
    uint32_t zero = 0;

    // 交换数据
    int new_fd = counters_fd_[new_active];
    int ret = bpf_map_update_elem(select_counter_fd_, &zero, &new_fd, BPF_ANY);
    if (ret < 0) {
        return -1;
    }

    // 交换锁
    int new_lock_fd = lock_fd_[new_active];
    ret = bpf_map_update_elem(select_lock_fd_, &zero, &new_lock_fd, BPF_ANY);
    if (ret < 0) {
        return -1;
    }

    current_active_ = new_active;
    return 0;
}

uint64_t MVSketchUser::query(const FlowKeyType& flow) const {
    // 获取快照 map 的 mmap 指针
    uint8_t* snapshot_map =
        reinterpret_cast<uint8_t*>(counters_mmap_[1 - current_active_]);

    uint64_t min_estimate = UINT64_MAX;

    for (int row = 0; row < MV_ROWS; row++) {
        uint32_t index = hash(&flow, row, MV_COLS);
        uint32_t offset = row * MV_COLS + index;

        // 需要用 stride 来对齐内存
        const MVBucket* bucket = reinterpret_cast<const MVBucket*>(
            snapshot_map + offset * MMAP_STRIDE(MVBucket));

        uint64_t estimate;
        if (std::memcmp(&bucket->flow_id, &flow, sizeof(FlowKeyType)) == 0) {
            // 桶中存储的就是查询流
            // 估计值 = (value + count) / 2
            uint64_t value = bucket->value;
            int32_t count = bucket->count;
            estimate = (value + static_cast<uint64_t>(count)) / 2;
        } else {
            // 桶中存储的不是查询流
            // 估计值 = (value - count) / 2
            uint64_t value = bucket->value;
            int32_t count = bucket->count;
            // 确保 value >= count，避免下溢
            if (value >= static_cast<uint64_t>(count)) {
                estimate = (value - static_cast<uint64_t>(count)) / 2;
            } else {
                estimate = 0;
            }
        }

        min_estimate = std::min(min_estimate, estimate);
    }

    return min_estimate == UINT64_MAX ? 0 : min_estimate;
}

void MVSketchUser::clear() {
    uint8_t* snapshot_map =
        reinterpret_cast<uint8_t*>(counters_mmap_[1 - current_active_]);
    // 清空整个 mmap 区域
    std::memset(snapshot_map, 0, MV_ROWS * MV_COLS * MMAP_STRIDE(MVBucket));
}
