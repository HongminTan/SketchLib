#include "FlowRadar.h"

FlowRadarUser::FlowRadarUser()
    : skel_(nullptr),
      select_bf_fd_(-1),
      select_ct_fd_(-1),
      select_ct_lock_fd_(-1),
      current_active_(0),
      ifindex_(0),
      is_decoded_(false) {
    bf_fd_[0] = -1;
    bf_fd_[1] = -1;
    bf_mmap_[0] = nullptr;
    bf_mmap_[1] = nullptr;
    ct_fd_[0] = -1;
    ct_fd_[1] = -1;
    ct_mmap_[0] = nullptr;
    ct_mmap_[1] = nullptr;
    ct_lock_fd_[0] = -1;
    ct_lock_fd_[1] = -1;

    skel_ = FlowRadar::open_and_load();
    if (!skel_) {
        exit(-1);
    }

    // 初始化 BloomFilter
    select_bf_fd_ = bpf_map__fd(skel_->maps.select_bf);
    if (select_bf_fd_ < 0) {
        FlowRadar::destroy(skel_);
        exit(-1);
    }

    bf_fd_[0] = bpf_map__fd(skel_->maps.bf_0);
    if (bf_fd_[0] < 0) {
        FlowRadar::destroy(skel_);
        exit(-1);
    }
    bf_fd_[1] = bpf_map__fd(skel_->maps.bf_1);
    if (bf_fd_[1] < 0) {
        FlowRadar::destroy(skel_);
        exit(-1);
    }

    // 初始化 CountingTable 数据
    select_ct_fd_ = bpf_map__fd(skel_->maps.select_ct);
    if (select_ct_fd_ < 0) {
        FlowRadar::destroy(skel_);
        exit(-1);
    }

    ct_fd_[0] = bpf_map__fd(skel_->maps.ct_0);
    if (ct_fd_[0] < 0) {
        FlowRadar::destroy(skel_);
        exit(-1);
    }
    ct_fd_[1] = bpf_map__fd(skel_->maps.ct_1);
    if (ct_fd_[1] < 0) {
        FlowRadar::destroy(skel_);
        exit(-1);
    }

    // 初始化 CountingTable 锁
    select_ct_lock_fd_ = bpf_map__fd(skel_->maps.select_ct_lock);
    if (select_ct_lock_fd_ < 0) {
        FlowRadar::destroy(skel_);
        exit(-1);
    }

    ct_lock_fd_[0] = bpf_map__fd(skel_->maps.ct_locks_0);
    if (ct_lock_fd_[0] < 0) {
        FlowRadar::destroy(skel_);
        exit(-1);
    }
    ct_lock_fd_[1] = bpf_map__fd(skel_->maps.ct_locks_1);
    if (ct_lock_fd_[1] < 0) {
        FlowRadar::destroy(skel_);
        exit(-1);
    }

    // mmap BloomFilter
    size_t bf_mmap_size = FR_BF_NUM_WORDS * MMAP_STRIDE(uint32_t);
    bf_mmap_[0] = static_cast<uint32_t*>(mmap(nullptr, bf_mmap_size,
                                              PROT_READ | PROT_WRITE,
                                              MAP_SHARED, bf_fd_[0], 0));
    if (bf_mmap_[0] == MAP_FAILED) {
        FlowRadar::destroy(skel_);
        exit(-1);
    }
    bf_mmap_[1] = static_cast<uint32_t*>(mmap(nullptr, bf_mmap_size,
                                              PROT_READ | PROT_WRITE,
                                              MAP_SHARED, bf_fd_[1], 0));
    if (bf_mmap_[1] == MAP_FAILED) {
        munmap(bf_mmap_[0], bf_mmap_size);
        FlowRadar::destroy(skel_);
        exit(-1);
    }

    // mmap CountingTable
    size_t ct_mmap_size = FR_CT_SIZE * MMAP_STRIDE(FRBucket);
    ct_mmap_[0] = static_cast<FRBucket*>(mmap(nullptr, ct_mmap_size,
                                              PROT_READ | PROT_WRITE,
                                              MAP_SHARED, ct_fd_[0], 0));
    if (ct_mmap_[0] == MAP_FAILED) {
        munmap(bf_mmap_[0], bf_mmap_size);
        munmap(bf_mmap_[1], bf_mmap_size);
        FlowRadar::destroy(skel_);
        exit(-1);
    }
    ct_mmap_[1] = static_cast<FRBucket*>(mmap(nullptr, ct_mmap_size,
                                              PROT_READ | PROT_WRITE,
                                              MAP_SHARED, ct_fd_[1], 0));
    if (ct_mmap_[1] == MAP_FAILED) {
        munmap(bf_mmap_[0], bf_mmap_size);
        munmap(bf_mmap_[1], bf_mmap_size);
        munmap(ct_mmap_[0], ct_mmap_size);
        FlowRadar::destroy(skel_);
        exit(-1);
    }

    current_active_ = 0;
}

FlowRadarUser::~FlowRadarUser() {
    detach();

    // 取消 mmap 映射
    size_t bf_mmap_size = FR_BF_NUM_WORDS * MMAP_STRIDE(uint32_t);
    size_t ct_mmap_size = FR_CT_SIZE * MMAP_STRIDE(FRBucket);

    if (bf_mmap_[0] != nullptr && bf_mmap_[0] != MAP_FAILED) {
        munmap(bf_mmap_[0], bf_mmap_size);
    }
    if (bf_mmap_[1] != nullptr && bf_mmap_[1] != MAP_FAILED) {
        munmap(bf_mmap_[1], bf_mmap_size);
    }
    if (ct_mmap_[0] != nullptr && ct_mmap_[0] != MAP_FAILED) {
        munmap(ct_mmap_[0], ct_mmap_size);
    }
    if (ct_mmap_[1] != nullptr && ct_mmap_[1] != MAP_FAILED) {
        munmap(ct_mmap_[1], ct_mmap_size);
    }

    if (skel_) {
        FlowRadar::destroy(skel_);
        skel_ = nullptr;
    }
}

int FlowRadarUser::attach(const char* ifname) {
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

void FlowRadarUser::detach() {
    if (ifindex_ == 0) {
        return;
    }
    bpf_xdp_detach(ifindex_, XDP_FLAGS_UPDATE_IF_NOEXIST, nullptr);
    ifindex_ = 0;
}

int FlowRadarUser::swap() {
    int new_active = 1 - current_active_;
    uint32_t zero = 0;

    // 交换 BloomFilter
    int new_bf_fd = bf_fd_[new_active];
    int ret = bpf_map_update_elem(select_bf_fd_, &zero, &new_bf_fd, BPF_ANY);
    if (ret < 0) {
        return -1;
    }

    // 交换 CountingTable 数据
    int new_ct_fd = ct_fd_[new_active];
    ret = bpf_map_update_elem(select_ct_fd_, &zero, &new_ct_fd, BPF_ANY);
    if (ret < 0) {
        return -1;
    }

    // 交换 CountingTable 锁
    int new_ct_lock_fd = ct_lock_fd_[new_active];
    ret = bpf_map_update_elem(select_ct_lock_fd_, &zero, &new_ct_lock_fd,
                              BPF_ANY);
    if (ret < 0) {
        return -1;
    }

    current_active_ = new_active;
    is_decoded_ = false;
    return 0;
}

std::map<FlowKeyType, uint64_t> FlowRadarUser::decode(int snapshot_idx) const {
    // 获取 CountingTable 快照
    uint8_t* ct_snapshot = reinterpret_cast<uint8_t*>(ct_mmap_[snapshot_idx]);

    // 创建 counting_table 的副本用于解码
    std::vector<FRBucket> ct_copy(FR_CT_SIZE);
    for (size_t i = 0; i < FR_CT_SIZE; i++) {
        const FRBucket* bucket = reinterpret_cast<const FRBucket*>(
            ct_snapshot + i * MMAP_STRIDE(FRBucket));
        ct_copy[i] = *bucket;
    }

    std::map<FlowKeyType, uint64_t> result;

    // 迭代解码
    while (true) {
        bool found_pure_bucket = false;

        // 查找纯桶（flow_count == 1）
        for (size_t i = 0; i < ct_copy.size(); i++) {
            if (ct_copy[i].flow_count == 1) {
                // 找到纯桶，直接解码
                FlowKeyType flow = ct_copy[i].flow_xor;
                uint32_t packet_count = ct_copy[i].packet_count;

                // 保存结果
                result[flow] = packet_count;

                // 从所有相关桶中减去这个流
                for (int j = 0; j < FR_CT_NUM_HASHES; j++) {
                    uint32_t index = hash(&flow, j, FR_CT_SIZE);

                    // XOR 流ID
                    uint8_t* dst =
                        reinterpret_cast<uint8_t*>(&ct_copy[index].flow_xor);
                    const uint8_t* src =
                        reinterpret_cast<const uint8_t*>(&flow);
                    for (size_t k = 0; k < sizeof(FlowKeyType); k++) {
                        dst[k] ^= src[k];
                    }

                    ct_copy[index].flow_count--;
                    ct_copy[index].packet_count -= packet_count;
                }

                found_pure_bucket = true;
            }
        }

        // 没有找到纯桶，解码结束
        if (!found_pure_bucket) {
            break;
        }
    }

    return result;
}

uint64_t FlowRadarUser::query(const FlowKeyType& flow) const {
    int snapshot_idx = 1 - current_active_;

    // 如果还没解码，先解码
    if (!is_decoded_) {
        decoded_map_ = decode(snapshot_idx);
        is_decoded_ = true;
    }

    // 从解码结果中查找
    auto it = decoded_map_.find(flow);
    if (it != decoded_map_.end()) {
        return it->second;
    }

    // 找不到返回 0
    return 0;
}

void FlowRadarUser::clear() {
    int snapshot_idx = 1 - current_active_;

    // 清空 BloomFilter
    size_t bf_mmap_size = FR_BF_NUM_WORDS * MMAP_STRIDE(uint32_t);
    std::memset(bf_mmap_[snapshot_idx], 0, bf_mmap_size);

    // 清空 CountingTable
    size_t ct_mmap_size = FR_CT_SIZE * MMAP_STRIDE(FRBucket);
    std::memset(ct_mmap_[snapshot_idx], 0, ct_mmap_size);

    // 清空解码缓存
    decoded_map_.clear();
    is_decoded_ = false;
}
