#ifndef COUNTSKETCH_H
#define COUNTSKETCH_H

#include "Config.h"
#include "CountSketch.skel.h"
#include "FlowKey.h"
#include "Sketch.h"
#include "hash.h"

#include <bpf/bpf.h>
#include <linux/if_link.h>
#include <net/if.h>
#include <sys/mman.h>
#include <algorithm>
#include <cstring>

class CountSketchUser : public Sketch {
   private:
    struct CountSketch* skel_;
    int select_counter_fd_;
    int counters_fd_[2];
    CS_COUNTER_TYPE* counters_mmap_[2];
    // 内核正在使用的 map
    int current_active_;
    // 挂载网卡的索引
    unsigned int ifindex_;

   public:
    CountSketchUser();
    ~CountSketchUser();

    // 禁止拷贝
    CountSketchUser(const CountSketchUser&) = delete;
    CountSketchUser& operator=(const CountSketchUser&) = delete;

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
     * @brief 从用户 buffer 中查询流的计数
     */
    uint64_t query(const FlowKeyType& flow) const override;

    /**
     * @brief 清空用户 buffer
     */
    void clear() override;
};

#endif  // COUNTSKETCH_H
