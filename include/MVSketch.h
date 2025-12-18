#ifndef MVSKETCH_H
#define MVSKETCH_H

#include "FlowKey.h"

#ifdef __BPF__
#include <bpf/bpf_helpers.h>
#endif

// MVSketch 桶结构
struct MVBucket {
    // 当前存储的候选流标识符
    FlowKeyType flow_id;
    // 所有映射到该桶的流的总计数
    uint32_t value;
    // 当前候选流的净计数
    int32_t count;
};

#ifdef __BPF__
// MVSketch 内核态自旋锁
struct MVBucketLock {
    struct bpf_spin_lock lock;
};
#endif

#ifndef __BPF__
#include "Config.h"
#include "MVSketch.skel.h"
#include "Sketch.h"
#include "hash.h"

#include <bpf/bpf.h>
#include <linux/if_link.h>
#include <net/if.h>
#include <sys/mman.h>
#include <algorithm>
#include <cstring>

class MVSketchUser : public Sketch {
   private:
    struct MVSketch* skel_;

    // 数据 map
    int select_counter_fd_;
    int counters_fd_[2];
    MVBucket* counters_mmap_[2];

    // 锁 map
    int select_lock_fd_;
    int lock_fd_[2];

    // 内核正在使用的 map
    int current_active_;
    // 挂载网卡的索引
    unsigned int ifindex_;

   public:
    MVSketchUser();
    ~MVSketchUser();

    // 禁止拷贝
    MVSketchUser(const MVSketchUser&) = delete;
    MVSketchUser& operator=(const MVSketchUser&) = delete;

    struct MVSketch* get_skel() const { return skel_; }

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

#endif  // MVSKETCH_H
