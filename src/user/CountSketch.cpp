#include "CountSketch.h"

CountSketchUser::CountSketchUser()
    : skel_(nullptr), select_counter_fd_(-1), current_active_(0), ifindex_(0) {
    counters_fd_[0] = -1;
    counters_fd_[1] = -1;
    counters_mmap_[0] = nullptr;
    counters_mmap_[1] = nullptr;

    skel_ = CountSketch::open_and_load();
    if (!skel_) {
        exit(-1);
    }

    // 拿到 select_counter 的描述符
    select_counter_fd_ = bpf_map__fd(skel_->maps.select_counter);
    if (select_counter_fd_ < 0) {
        CountSketch::destroy(skel_);
        exit(-1);
    }

    // 拿到 double buffer 的描述符
    counters_fd_[0] = bpf_map__fd(skel_->maps.counters_0);
    if (counters_fd_[0] < 0) {
        CountSketch::destroy(skel_);
        exit(-1);
    }
    counters_fd_[1] = bpf_map__fd(skel_->maps.counters_1);
    if (counters_fd_[1] < 0) {
        CountSketch::destroy(skel_);
        exit(-1);
    }

    // 计算 map 在内存中的实际排布
    size_t mmap_size = CS_ROWS * CS_COLS * MMAP_STRIDE(CS_COUNTER_TYPE);
    // mmap 映射两个 counter
    counters_mmap_[0] = static_cast<CS_COUNTER_TYPE*>(
        mmap(nullptr, mmap_size, PROT_READ | PROT_WRITE, MAP_SHARED,
             counters_fd_[0], 0));
    if (counters_mmap_[0] == MAP_FAILED) {
        CountSketch::destroy(skel_);
        exit(-1);
    }
    counters_mmap_[1] = static_cast<CS_COUNTER_TYPE*>(
        mmap(nullptr, mmap_size, PROT_READ | PROT_WRITE, MAP_SHARED,
             counters_fd_[1], 0));
    if (counters_mmap_[1] == MAP_FAILED) {
        munmap(counters_mmap_[0], mmap_size);
        CountSketch::destroy(skel_);
        exit(-1);
    }

    current_active_ = 0;
}

CountSketchUser::~CountSketchUser() {
    detach();

    // 取消 mmap 映射
    size_t mmap_size = CS_ROWS * CS_COLS * MMAP_STRIDE(CS_COUNTER_TYPE);
    if (counters_mmap_[0] != nullptr && counters_mmap_[0] != MAP_FAILED) {
        munmap(counters_mmap_[0], mmap_size);
    }
    if (counters_mmap_[1] != nullptr && counters_mmap_[1] != MAP_FAILED) {
        munmap(counters_mmap_[1], mmap_size);
    }

    if (skel_) {
        CountSketch::destroy(skel_);
        skel_ = nullptr;
    }
}

int CountSketchUser::attach(const char* ifname) {
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

void CountSketchUser::detach() {
    if (ifindex_ == 0) {
        return;
    }
    bpf_xdp_detach(ifindex_, XDP_FLAGS_UPDATE_IF_NOEXIST, nullptr);
    ifindex_ = 0;
}

int CountSketchUser::swap() {
    int new_active = 1 - current_active_;
    int new_fd = counters_fd_[new_active];
    uint32_t zero = 0;

    int ret = bpf_map_update_elem(select_counter_fd_, &zero, &new_fd, BPF_ANY);
    if (ret < 0) {
        return -1;
    }

    current_active_ = new_active;
    return counters_fd_[1 - new_active];
}

uint64_t CountSketchUser::query(const FlowKeyType& flow) const {
    // 获取快照 map 的 mmap 指针
    uint8_t* snapshot_map =
        reinterpret_cast<uint8_t*>(counters_mmap_[1 - current_active_]);

    int64_t estimates[CS_ROWS];

    for (int row = 0; row < CS_ROWS; row++) {
        uint32_t index = hash(&flow, row, CS_COLS);
        uint32_t offset = row * CS_COLS + index;

        // 需要用 stride 来对齐内存
        CS_COUNTER_TYPE value = *reinterpret_cast<CS_COUNTER_TYPE*>(
            snapshot_map + offset * MMAP_STRIDE(CS_COUNTER_TYPE));

        // 存储行内值
        uint32_t sign_hash = hash(&flow, row + CS_ROWS, 2);
        estimates[row] = sign_hash ? value : -value;
    }

    // 取中位数
    std::sort(estimates, estimates + CS_ROWS);
    uint64_t median_index = CS_ROWS / 2;
    int64_t median_value;
    if (CS_ROWS % 2 == 0) {
        median_value =
            (estimates[median_index] + estimates[median_index - 1]) / 2;
    } else {
        median_value = estimates[median_index];
    }

    return static_cast<uint64_t>(std::max(int64_t(0), median_value));
}

void CountSketchUser::clear() {
    uint8_t* snapshot_map =
        reinterpret_cast<uint8_t*>(counters_mmap_[1 - current_active_]);
    // 清空整个mmap区域
    std::memset(snapshot_map, 0,
                CS_ROWS * CS_COLS * MMAP_STRIDE(CS_COUNTER_TYPE));
}
