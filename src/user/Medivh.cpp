#include "Medivh.h"

MedivhUser::MedivhUser()
    : skel_(nullptr),
      select_table_fd_(-1),
      select_locks_fd_(-1),
      current_active_(0),
      ifindex_(0) {
    table_fd_[0] = table_fd_[1] = -1;
    locks_fd_[0] = locks_fd_[1] = -1;
    table_mmap_[0] = table_mmap_[1] = nullptr;

    // 初始化虚拟数据
    virtual_data_.resize(MD_NUM_SKETCHLETS);

    skel_ = Medivh::open_and_load();
    if (!skel_) {
        exit(-1);
    }

    // 初始化数据
    select_table_fd_ = bpf_map__fd(skel_->maps.select_table);
    if (select_table_fd_ < 0) {
        Medivh::destroy(skel_);
        exit(-1);
    }

    table_fd_[0] = bpf_map__fd(skel_->maps.table_0);
    table_fd_[1] = bpf_map__fd(skel_->maps.table_1);
    if (table_fd_[0] < 0 || table_fd_[1] < 0) {
        Medivh::destroy(skel_);
        exit(-1);
    }

    // 初始化锁
    select_locks_fd_ = bpf_map__fd(skel_->maps.select_locks);
    if (select_locks_fd_ < 0) {
        Medivh::destroy(skel_);
        exit(-1);
    }
    locks_fd_[0] = bpf_map__fd(skel_->maps.locks_0);
    locks_fd_[1] = bpf_map__fd(skel_->maps.locks_1);
    if (locks_fd_[0] < 0 || locks_fd_[1] < 0) {
        Medivh::destroy(skel_);
        exit(-1);
    }

    // mmap 映射哈希表
    size_t mmap_size = MD_HASH_TABLE_SIZE * MMAP_STRIDE(struct MedivhBucket);
    table_mmap_[0] = static_cast<struct MedivhBucket*>(
        mmap(nullptr, mmap_size, PROT_READ | PROT_WRITE, MAP_SHARED,
             table_fd_[0], 0));
    if (table_mmap_[0] == MAP_FAILED) {
        Medivh::destroy(skel_);
        exit(-1);
    }
    table_mmap_[1] = static_cast<struct MedivhBucket*>(
        mmap(nullptr, mmap_size, PROT_READ | PROT_WRITE, MAP_SHARED,
             table_fd_[1], 0));
    if (table_mmap_[1] == MAP_FAILED) {
        munmap(table_mmap_[0], mmap_size);
        Medivh::destroy(skel_);
        exit(-1);
    }

    current_active_ = 0;
}

MedivhUser::~MedivhUser() {
    detach();

    // 取消 mmap 映射
    size_t mmap_size = MD_HASH_TABLE_SIZE * MMAP_STRIDE(struct MedivhBucket);
    if (table_mmap_[0] != nullptr && table_mmap_[0] != MAP_FAILED) {
        munmap(table_mmap_[0], mmap_size);
    }
    if (table_mmap_[1] != nullptr && table_mmap_[1] != MAP_FAILED) {
        munmap(table_mmap_[1], mmap_size);
    }

    if (skel_) {
        Medivh::destroy(skel_);
        skel_ = nullptr;
    }
}

int MedivhUser::attach(const char* ifname) {
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

void MedivhUser::detach() {
    if (ifindex_ == 0) {
        return;
    }
    bpf_xdp_detach(ifindex_, XDP_FLAGS_UPDATE_IF_NOEXIST, nullptr);
    ifindex_ = 0;
}

int MedivhUser::swap() {
    int new_active = 1 - current_active_;
    uint32_t zero = 0;

    // 交换数据
    int new_table_fd = table_fd_[new_active];
    int ret =
        bpf_map_update_elem(select_table_fd_, &zero, &new_table_fd, BPF_ANY);
    if (ret < 0) {
        return -1;
    }

    // 交换锁
    int new_locks_fd = locks_fd_[new_active];
    ret = bpf_map_update_elem(select_locks_fd_, &zero, &new_locks_fd, BPF_ANY);
    if (ret < 0) {
        return -1;
    }

    current_active_ = new_active;
    return 0;
}

void MedivhUser::reconstruct() {
    // 清空虚拟数据
    for (auto& sketchlet : virtual_data_) {
        sketchlet.clear();
    }

    // 获取快照
    uint8_t* snapshot =
        reinterpret_cast<uint8_t*>(table_mmap_[1 - current_active_]);

    // 遍历哈希表重建
    for (uint32_t idx = 0; idx < MD_HASH_TABLE_SIZE; ++idx) {
        struct MedivhBucket* bucket = reinterpret_cast<struct MedivhBucket*>(
            snapshot + idx * MMAP_STRIDE(struct MedivhBucket));

        if (MD_IS_EMPTY(bucket->metadata))
            continue;

        uint8_t sketchlet_id = MD_GET_ID(bucket->metadata);
        if (sketchlet_id >= MD_NUM_SKETCHLETS)
            continue;

        uint32_t original_idx = bucket->original_idx;

        // 存储到虚拟 sketchlet
        auto& sketchlet_data = virtual_data_[sketchlet_id];
        auto it = sketchlet_data.find(original_idx);
        if (it == sketchlet_data.end()) {
            // 没有恢复过该槽位，直接插入
            sketchlet_data[original_idx] = bucket->data;
        } else {
            // 合并已恢复过的槽位数据和当前数据
            uint8_t type = sketchlet_types[sketchlet_id];
            switch (type) {
                case SKETCHLET_CM:
                    it->second.cm += bucket->data.cm;
                    break;
                case SKETCHLET_CS:
                    it->second.cs += bucket->data.cs;
                    break;
                default:
                    // MV/ES/FR 不需要合并，直接覆盖
                    it->second = bucket->data;
                    break;
            }
        }
    }
}

uint64_t MedivhUser::query_sketchlet(uint8_t sketchlet_id,
                                     const FlowKeyType& flow) const {
    if (sketchlet_id >= MD_NUM_SKETCHLETS)
        return 0;

    uint8_t type = sketchlet_types[sketchlet_id];
    uint32_t j = hash(&flow, sketchlet_id, MD_HASH_TABLE_SIZE);
    const auto& sketchlet_data = virtual_data_[sketchlet_id];
    auto it = sketchlet_data.find(j);

    if (it == sketchlet_data.end())
        return 0;

    switch (type) {
        case SKETCHLET_CM:
            return it->second.cm;
        case SKETCHLET_CS:
            return static_cast<uint64_t>(it->second.cs > 0 ? it->second.cs : 0);
        case SKETCHLET_MV:
            return it->second.mv.value;
        default:
            return 0;
    }
}

const union MedivhBucketData* MedivhUser::query_bucket(uint8_t sketchlet_id,
                                                       uint32_t idx) const {
    if (sketchlet_id >= MD_NUM_SKETCHLETS)
        return nullptr;

    const auto& sketchlet_data = virtual_data_[sketchlet_id];
    auto it = sketchlet_data.find(idx);
    if (it != sketchlet_data.end()) {
        return &it->second;
    }
    return nullptr;
}

uint64_t MedivhUser::query(const FlowKeyType& flow) const {
    uint64_t min_val = UINT64_MAX;

    for (uint8_t i = 0; i < MD_NUM_SKETCHLETS; i++) {
        uint64_t val = query_sketchlet(i, flow);
        if (val < min_val) {
            min_val = val;
        }
    }

    return (min_val == UINT64_MAX) ? 0 : min_val;
}

void MedivhUser::clear() {
    uint8_t* snapshot =
        reinterpret_cast<uint8_t*>(table_mmap_[1 - current_active_]);
    std::memset(snapshot, 0,
                MD_HASH_TABLE_SIZE * MMAP_STRIDE(struct MedivhBucket));

    for (auto& sk : virtual_data_) {
        sk.clear();
    }
}
