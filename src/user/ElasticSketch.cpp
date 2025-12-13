#include "ElasticSketch.h"

ElasticSketchUser::ElasticSketchUser()
    : skel_(nullptr),
      select_heavy_fd_(-1),
      select_heavy_lock_fd_(-1),
      select_light_fd_(-1),
      current_active_(0),
      ifindex_(0) {
    heavy_fd_[0] = -1;
    heavy_fd_[1] = -1;
    heavy_mmap_[0] = nullptr;
    heavy_mmap_[1] = nullptr;
    heavy_lock_fd_[0] = -1;
    heavy_lock_fd_[1] = -1;
    light_fd_[0] = -1;
    light_fd_[1] = -1;
    light_mmap_[0] = nullptr;
    light_mmap_[1] = nullptr;

    skel_ = ElasticSketch::open_and_load();
    if (!skel_) {
        exit(-1);
    }

    // 初始化 Heavy Part
    select_heavy_fd_ = bpf_map__fd(skel_->maps.select_heavy_part);
    if (select_heavy_fd_ < 0) {
        ElasticSketch::destroy(skel_);
        exit(-1);
    }

    heavy_fd_[0] = bpf_map__fd(skel_->maps.heavy_part_0);
    if (heavy_fd_[0] < 0) {
        ElasticSketch::destroy(skel_);
        exit(-1);
    }
    heavy_fd_[1] = bpf_map__fd(skel_->maps.heavy_part_1);
    if (heavy_fd_[1] < 0) {
        ElasticSketch::destroy(skel_);
        exit(-1);
    }

    // 初始化 Heavy Part 锁
    select_heavy_lock_fd_ = bpf_map__fd(skel_->maps.select_heavy_lock);
    if (select_heavy_lock_fd_ < 0) {
        ElasticSketch::destroy(skel_);
        exit(-1);
    }

    heavy_lock_fd_[0] = bpf_map__fd(skel_->maps.heavy_lock_0);
    if (heavy_lock_fd_[0] < 0) {
        ElasticSketch::destroy(skel_);
        exit(-1);
    }

    heavy_lock_fd_[1] = bpf_map__fd(skel_->maps.heavy_lock_1);
    if (heavy_lock_fd_[1] < 0) {
        ElasticSketch::destroy(skel_);
        exit(-1);
    }

    // mmap Heavy Part
    size_t heavy_mmap_size = ES_HEAVY_BUCKET_COUNT * MMAP_STRIDE(HeavyBucket);
    heavy_mmap_[0] = static_cast<HeavyBucket*>(
        mmap(nullptr, heavy_mmap_size, PROT_READ | PROT_WRITE, MAP_SHARED,
             heavy_fd_[0], 0));
    if (heavy_mmap_[0] == MAP_FAILED) {
        ElasticSketch::destroy(skel_);
        exit(-1);
    }
    heavy_mmap_[1] = static_cast<HeavyBucket*>(
        mmap(nullptr, heavy_mmap_size, PROT_READ | PROT_WRITE, MAP_SHARED,
             heavy_fd_[1], 0));
    if (heavy_mmap_[1] == MAP_FAILED) {
        munmap(heavy_mmap_[0], heavy_mmap_size);
        ElasticSketch::destroy(skel_);
        exit(-1);
    }

    // 初始化 Light Part
    select_light_fd_ = bpf_map__fd(skel_->maps.select_light_count);
    if (select_light_fd_ < 0) {
        munmap(heavy_mmap_[0], heavy_mmap_size);
        munmap(heavy_mmap_[1], heavy_mmap_size);
        ElasticSketch::destroy(skel_);
        exit(-1);
    }

    light_fd_[0] = bpf_map__fd(skel_->maps.light_counts_0);
    if (light_fd_[0] < 0) {
        munmap(heavy_mmap_[0], heavy_mmap_size);
        munmap(heavy_mmap_[1], heavy_mmap_size);
        ElasticSketch::destroy(skel_);
        exit(-1);
    }
    light_fd_[1] = bpf_map__fd(skel_->maps.light_counts_1);
    if (light_fd_[1] < 0) {
        munmap(heavy_mmap_[0], heavy_mmap_size);
        munmap(heavy_mmap_[1], heavy_mmap_size);
        ElasticSketch::destroy(skel_);
        exit(-1);
    }

    // mmap Light Part
    size_t light_mmap_size =
        ES_LIGHT_ROWS * ES_LIGHT_COLS * MMAP_STRIDE(ES_LIGHT_COUNTER_TYPE);
    light_mmap_[0] = static_cast<ES_LIGHT_COUNTER_TYPE*>(
        mmap(nullptr, light_mmap_size, PROT_READ | PROT_WRITE, MAP_SHARED,
             light_fd_[0], 0));
    if (light_mmap_[0] == MAP_FAILED) {
        munmap(heavy_mmap_[0], heavy_mmap_size);
        munmap(heavy_mmap_[1], heavy_mmap_size);
        ElasticSketch::destroy(skel_);
        exit(-1);
    }
    light_mmap_[1] = static_cast<ES_LIGHT_COUNTER_TYPE*>(
        mmap(nullptr, light_mmap_size, PROT_READ | PROT_WRITE, MAP_SHARED,
             light_fd_[1], 0));
    if (light_mmap_[1] == MAP_FAILED) {
        munmap(heavy_mmap_[0], heavy_mmap_size);
        munmap(heavy_mmap_[1], heavy_mmap_size);
        munmap(light_mmap_[0], light_mmap_size);
        ElasticSketch::destroy(skel_);
        exit(-1);
    }

    current_active_ = 0;
}

ElasticSketchUser::~ElasticSketchUser() {
    detach();

    // 取消 mmap 映射
    size_t heavy_mmap_size = ES_HEAVY_BUCKET_COUNT * MMAP_STRIDE(HeavyBucket);
    size_t light_mmap_size =
        ES_LIGHT_ROWS * ES_LIGHT_COLS * MMAP_STRIDE(ES_LIGHT_COUNTER_TYPE);

    if (heavy_mmap_[0] != nullptr && heavy_mmap_[0] != MAP_FAILED) {
        munmap(heavy_mmap_[0], heavy_mmap_size);
    }
    if (heavy_mmap_[1] != nullptr && heavy_mmap_[1] != MAP_FAILED) {
        munmap(heavy_mmap_[1], heavy_mmap_size);
    }
    if (light_mmap_[0] != nullptr && light_mmap_[0] != MAP_FAILED) {
        munmap(light_mmap_[0], light_mmap_size);
    }
    if (light_mmap_[1] != nullptr && light_mmap_[1] != MAP_FAILED) {
        munmap(light_mmap_[1], light_mmap_size);
    }

    if (skel_) {
        ElasticSketch::destroy(skel_);
        skel_ = nullptr;
    }
}

int ElasticSketchUser::attach(const char* ifname) {
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

void ElasticSketchUser::detach() {
    if (ifindex_ == 0) {
        return;
    }
    bpf_xdp_detach(ifindex_, XDP_FLAGS_UPDATE_IF_NOEXIST, nullptr);
    ifindex_ = 0;
}

int ElasticSketchUser::swap() {
    int new_active = 1 - current_active_;
    uint32_t zero = 0;

    // 交换 Heavy Part
    int new_heavy_fd = heavy_fd_[new_active];
    int ret =
        bpf_map_update_elem(select_heavy_fd_, &zero, &new_heavy_fd, BPF_ANY);
    if (ret < 0) {
        return -1;
    }

    // 交换 Heavy Part 锁
    int new_heavy_lock_fd = heavy_lock_fd_[new_active];
    ret = bpf_map_update_elem(select_heavy_lock_fd_, &zero, &new_heavy_lock_fd,
                              BPF_ANY);
    if (ret < 0) {
        return -1;
    }

    // 交换 Light Part
    int new_light_fd = light_fd_[new_active];
    ret = bpf_map_update_elem(select_light_fd_, &zero, &new_light_fd, BPF_ANY);
    if (ret < 0) {
        return -1;
    }

    current_active_ = new_active;
    return 0;
}

uint64_t ElasticSketchUser::query(const FlowKeyType& flow) const {
    int snapshot_idx = 1 - current_active_;

    // 查询 Heavy Part
    uint32_t bucket_idx = hash(&flow, ES_HEAVY_SEED, ES_HEAVY_BUCKET_COUNT);

    // 获取 Heavy Part 快照
    uint8_t* heavy_snapshot =
        reinterpret_cast<uint8_t*>(heavy_mmap_[snapshot_idx]);
    const HeavyBucket* bucket = reinterpret_cast<const HeavyBucket*>(
        heavy_snapshot + bucket_idx * MMAP_STRIDE(HeavyBucket));

    // 检查是否是同一个流
    if (std::memcmp(&bucket->flow_id, &flow, sizeof(FlowKeyType)) == 0) {
        uint64_t heavy_count = bucket->pos_vote;

        // 如果该桶发生过替换，需要加上 Light Part 的计数
        if (bucket->flag) {
            uint64_t light_count = query_light_part(flow, snapshot_idx);
            return heavy_count + light_count;
        }

        return heavy_count;
    }

    // 流不在 Heavy Part 中，查询 Light Part
    return query_light_part(flow, snapshot_idx);
}

void ElasticSketchUser::clear() {
    int snapshot_idx = 1 - current_active_;

    // 清空 Heavy Part
    size_t heavy_mmap_size = ES_HEAVY_BUCKET_COUNT * MMAP_STRIDE(HeavyBucket);
    std::memset(heavy_mmap_[snapshot_idx], 0, heavy_mmap_size);

    // 清空 Light Part
    size_t light_mmap_size =
        ES_LIGHT_ROWS * ES_LIGHT_COLS * MMAP_STRIDE(ES_LIGHT_COUNTER_TYPE);
    std::memset(light_mmap_[snapshot_idx], 0, light_mmap_size);
}

uint64_t ElasticSketchUser::query_light_part(const FlowKeyType& flow,
                                             int snapshot_idx) const {
    uint8_t* light_snapshot =
        reinterpret_cast<uint8_t*>(light_mmap_[snapshot_idx]);

    uint64_t min_count = UINT64_MAX;

    for (int row = 0; row < ES_LIGHT_ROWS; row++) {
        uint32_t index = hash(&flow, row, ES_LIGHT_COLS);
        uint32_t offset = row * ES_LIGHT_COLS + index;

        ES_LIGHT_COUNTER_TYPE value = *reinterpret_cast<ES_LIGHT_COUNTER_TYPE*>(
            light_snapshot + offset * MMAP_STRIDE(ES_LIGHT_COUNTER_TYPE));

        if (value < min_count) {
            min_count = value;
        }
    }

    return min_count == UINT64_MAX ? 0 : min_count;
}
