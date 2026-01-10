#ifndef MEDIVH_H
#define MEDIVH_H

#include "Config.h"
#include "ElasticSketch.h"
#include "FlowKey.h"
#include "FlowRadar.h"
#include "MVSketch.h"

// Union Bucket - 兼容众多 Sketch Bucket
union MedivhBucketData {
    // CountMin
    CM_COUNTER_TYPE cm;
    // CountSketch
    CS_COUNTER_TYPE cs;
    // MVSketch
    struct MVBucket mv;
    // ElasticSketch HeavyPart
    struct HeavyBucket es;
    // FlowRadar
    struct FRBucket fr;
};

// Medivh 桶结构

struct MedivhBucket {
    /** metadata 字段布局:
     *   bit 31: 占用标志 0=空闲, 1=已占用
     *   bit [15:8]: bucket 对应的 sketch type
     *   bit [7:0]:  bucket 对应的 sketchlet id
     */
    uint32_t metadata;
    // 记录冲突处理前的原始 index
    uint32_t original_idx;
    union MedivhBucketData data;
};

// metadata 字段操作宏
#define MD_OCCUPIED_BIT 0x80000000
#define MD_MAKE_METADATA(type, id) \
    (MD_OCCUPIED_BIT | (((uint32_t)(type) << 8) | ((uint32_t)(id))))
#define MD_GET_TYPE(type_id) (((type_id) >> 8) & 0xFF)
#define MD_GET_ID(type_id) ((type_id) & 0xFF)
#define MD_IS_OCCUPIED(type_id) (((type_id) & MD_OCCUPIED_BIT) != 0)
#define MD_IS_EMPTY(type_id) (((type_id) & MD_OCCUPIED_BIT) == 0)

// 计算哈希表大小
#define MD_BUCKET_SIZE sizeof(struct MedivhBucket)
#define MD_HASH_TABLE_SIZE (MD_MEMORY / MD_BUCKET_SIZE)

#ifdef __BPF__
#include <bpf/bpf_helpers.h>

struct MedivhBucketLock {
    struct bpf_spin_lock lock;
};

// sketchlet 类型映射数组
static const uint8_t sketchlet_types[MD_NUM_SKETCHLETS] = MD_SKETCHLET_TYPES;
#endif  // __BPF__

#ifndef __BPF__
#include "Medivh.skel.h"
#include "Sketch.h"
#include "hash.h"

#include <bpf/bpf.h>
#include <linux/if_link.h>
#include <net/if.h>
#include <sys/mman.h>
#include <cstring>
#include <unordered_map>
#include <vector>

static const uint8_t sketchlet_types[MD_NUM_SKETCHLETS] = MD_SKETCHLET_TYPES;

class MedivhUser : public Sketch {
   private:
    struct Medivh* skel_;

    // 哈希表 map
    int select_table_fd_;
    int table_fd_[2];
    struct MedivhBucket* table_mmap_[2];

    // 锁 map
    int select_locks_fd_;
    int locks_fd_[2];

    // 内核正在使用的 map
    int current_active_;
    // 挂载网卡的索引
    unsigned int ifindex_;

    // 重建后的虚拟 sketchlet 数据
    std::vector<std::unordered_map<uint32_t, union MedivhBucketData>>
        virtual_data_;

   public:
    MedivhUser();
    ~MedivhUser();

    // 禁止拷贝
    MedivhUser(const MedivhUser&) = delete;
    MedivhUser& operator=(const MedivhUser&) = delete;

    struct Medivh* get_skel() const { return skel_; }

    /**
     * @brief 挂载内核态程序到网卡
     * @param ifname 网卡名称
     * @return 0 成功，负数失败
     */
    int attach(const char* ifname);

    /**
     * @brief 从网卡卸载内核态程序
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

    /**
     * @brief 从哈希表重建虚拟 sketchlet
     */
    void reconstruct();

    /**
     * @brief 查询指定 sketchlet 的指定槽位
     * @param sketchlet_id sketchlet 索引
     * @param idx 槽位索引
     * @return 槽位数据指针，未找到返回 nullptr
     */
    const union MedivhBucketData* query_bucket(uint8_t sketchlet_id,
                                               uint32_t idx) const;

    /**
     * @brief 查询指定 sketchlet 中流的计数
     * @param sketchlet_id sketchlet 索引
     * @param flow 流标识符
     * @return 估计计数
     */
    uint64_t query_sketchlet(uint8_t sketchlet_id,
                             const FlowKeyType& flow) const;
};

#endif  // __BPF__

#endif  // MEDIVH_H
