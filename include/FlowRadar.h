#ifndef FLOWRADAR_H
#define FLOWRADAR_H

#include "FlowKey.h"

#ifdef __BPF__
#include <bpf/bpf_helpers.h>
#endif

// FlowRadar 桶结构
struct FRBucket {
    // 流ID的异或和
    FlowKeyType flow_xor;
    // 流数量
    uint32_t flow_count;
    // 包数量
    uint32_t packet_count;
};

#ifdef __BPF__
// FlowRadar 内核态自旋锁
struct FRBucketLock {
    struct bpf_spin_lock lock;
};
#endif

#ifndef __BPF__
#include "Config.h"
#include "FlowRadar.skel.h"
#include "Sketch.h"
#include "hash.h"

#include <bpf/bpf.h>
#include <linux/if_link.h>
#include <net/if.h>
#include <sys/mman.h>
#include <algorithm>
#include <cstring>
#include <map>
#include <vector>

class FlowRadarUser : public Sketch {
   private:
    struct FlowRadar* skel_;

    // BloomFilter map
    int select_bf_fd_;
    int bf_fd_[2];
    uint32_t* bf_mmap_[2];

    // CountingTable 数据 map
    int select_ct_fd_;
    int ct_fd_[2];
    FRBucket* ct_mmap_[2];

    // CountingTable 锁 map
    int select_ct_lock_fd_;
    int ct_lock_fd_[2];

    // 内核正在使用的 map
    int current_active_;
    // 挂载网卡的索引
    unsigned int ifindex_;

    // 解码结果缓存
    mutable std::map<FlowKeyType, uint64_t> decoded_map_;
    mutable bool is_decoded_;

    // 解码辅助函数
    std::map<FlowKeyType, uint64_t> decode(int snapshot_idx) const;

   public:
    FlowRadarUser();
    ~FlowRadarUser();

    // 禁止拷贝
    FlowRadarUser(const FlowRadarUser&) = delete;
    FlowRadarUser& operator=(const FlowRadarUser&) = delete;

    struct FlowRadar* get_skel() const { return skel_; }

    /**
     * @brief 挂载到网卡
     * @param ifname 网卡名称
     * @return 0 成功，负数失败
     */
    int attach(const char* ifname);

    /**
     * @brief 从网卡卸载
     */
    void detach();

    /**
     * @brief 交换用户 buffer 与内核 buffer
     * @return 0 成功，-1 失败
     */
    int swap();

    /**
     * @brief 查询流的估计计数
     */
    uint64_t query(const FlowKeyType& flow) const override;

    /**
     * @brief 清空用户 buffer
     */
    void clear() override;
};

#endif  // __BPF__

#endif  // FLOWRADAR_H
