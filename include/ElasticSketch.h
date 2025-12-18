#ifndef ELASTICSKETCH_H
#define ELASTICSKETCH_H

#include "FlowKey.h"

#ifdef __BPF__
#include <bpf/bpf_helpers.h>
#endif

// ElasticSketch Heavy Part 桶结构
struct HeavyBucket {
    // 流标识符
    FlowKeyType flow_id;
    // 正票
    uint32_t pos_vote;
    // 负票
    uint32_t neg_vote;
    // 该桶是否发生过替换
    uint8_t flag;
    // 对齐
    uint8_t padding[3];
};

#ifdef __BPF__
// Heavy Part 内核态自旋锁
struct HeavyBucketLock {
    struct bpf_spin_lock lock;
};
#endif

#ifndef __BPF__
#include "Config.h"
#include "ElasticSketch.skel.h"
#include "Sketch.h"
#include "hash.h"

#include <bpf/bpf.h>
#include <linux/if_link.h>
#include <net/if.h>
#include <sys/mman.h>
#include <algorithm>
#include <cstring>

class ElasticSketchUser : public Sketch {
   private:
    struct ElasticSketch* skel_;

    // Heavy Part
    int select_heavy_fd_;
    int heavy_fd_[2];
    HeavyBucket* heavy_mmap_[2];

    // Heavy Part 锁
    int select_heavy_lock_fd_;
    int heavy_lock_fd_[2];

    // Light Part
    int select_light_fd_;
    int light_fd_[2];
    ES_LIGHT_COUNTER_TYPE* light_mmap_[2];

    // 内核正在使用的 map
    int current_active_;
    // 挂载网卡的索引
    unsigned int ifindex_;

    // 查询 Light Part
    uint64_t query_light_part(const FlowKeyType& flow, int snapshot_idx) const;

   public:
    ElasticSketchUser();
    ~ElasticSketchUser();

    // 禁止拷贝
    ElasticSketchUser(const ElasticSketchUser&) = delete;
    ElasticSketchUser& operator=(const ElasticSketchUser&) = delete;

    struct ElasticSketch* get_skel() const { return skel_; }

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

#endif  // ELASTICSKETCH_H
