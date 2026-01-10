// 开启 BPF 编译模式
#define __BPF__ 1

#include "Medivh.h"
#include "Config.h"
#include "FlowKey.h"
#include "autogen/vmlinux.h"
#include "hash.h"

#include <bpf/bpf_helpers.h>

// Medivh 数据 Maps
struct medivh_table {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(max_entries, MD_HASH_TABLE_SIZE);
    __uint(map_flags, BPF_F_MMAPABLE);
    __type(key, uint32_t);
    __type(value, struct MedivhBucket);
};

// 数据 double buffer
struct medivh_table table_0 SEC(".maps");
struct medivh_table table_1 SEC(".maps");

// 数据 buffer 选择 map
struct {
    __uint(type, BPF_MAP_TYPE_ARRAY_OF_MAPS);
    __uint(max_entries, 1);
    __type(key, uint32_t);
    __array(values, struct medivh_table);
} select_table SEC(".maps") = {
    .values = {&table_0},
};

// Medivh 锁 Maps
struct medivh_locks {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(max_entries, MD_HASH_TABLE_SIZE);
    __type(key, uint32_t);
    __type(value, struct MedivhBucketLock);
};

// 锁 double buffer
struct medivh_locks locks_0 SEC(".maps");
struct medivh_locks locks_1 SEC(".maps");

// 锁 buffer 选择 map
struct {
    __uint(type, BPF_MAP_TYPE_ARRAY_OF_MAPS);
    __uint(max_entries, 1);
    __type(key, uint32_t);
    __array(values, struct medivh_locks);
} select_locks SEC(".maps") = {
    .values = {&locks_0},
};

static __always_inline void update_cm(union MedivhBucketData* data) {
    if (data->cm < UINT32_MAX) {
        __sync_fetch_and_add(&data->cm, 1);
    }
}

static __always_inline void update_cs(union MedivhBucketData* data,
                                      int32_t increment) {
    if ((increment > 0 && data->cs < INT32_MAX) ||
        (increment < 0 && data->cs > INT32_MIN)) {
        __sync_fetch_and_add(&data->cs, increment);
    }
}

static __always_inline void update_mv(union MedivhBucketData* data,
                                      const FlowKeyType* flow) {
    data->mv.value++;

    FlowKeyType empty = {};
    if (__builtin_memcmp(&data->mv.flow_id, &empty, sizeof(FlowKeyType)) == 0) {
        // 空桶，插入新流
        __builtin_memcpy(&data->mv.flow_id, flow, sizeof(FlowKeyType));
        data->mv.count = 1;
    } else if (__builtin_memcmp(&data->mv.flow_id, flow, sizeof(FlowKeyType)) ==
               0) {
        // 同一流，增加计数
        if (data->mv.count < INT32_MAX) {
            data->mv.count++;
        }
    } else {
        // 不同流，减少计数
        if (data->mv.count > INT32_MIN) {
            data->mv.count--;
        }
        // 如果 count < 0，说明更新流更占优，替换候选流
        if (data->mv.count <= 0) {
            __builtin_memcpy(&data->mv.flow_id, flow, sizeof(FlowKeyType));
            data->mv.count = 1;
        }
    }
}

// 判断 Sketchlet 类型是否需要锁来保护数据竞争
static __always_inline int needs_lock(uint8_t type) {
    return (type == SKETCHLET_MV || type == SKETCHLET_ES ||
            type == SKETCHLET_FR);
}

SEC("xdp")
int update(struct xdp_md* ctx) {
    FlowKeyType key;
    if (extract_flowkey(ctx, &key))
        return XDP_PASS;

    uint32_t zero = 0;
    void* table = bpf_map_lookup_elem(&select_table, &zero);
    void* locks = bpf_map_lookup_elem(&select_locks, &zero);
    if (!table || !locks)
        return XDP_PASS;

    // 遍历 sketchlets
#pragma unroll
    for (uint8_t i = 0; i < MD_NUM_SKETCHLETS; i++) {
        uint8_t type = sketchlet_types[i];
        if (type == SKETCHLET_EMPTY)
            continue;

        // 计算原始哈希索引
        uint32_t j = hash(&key, i, MD_HASH_TABLE_SIZE);

        // CS 需要预先计算符号位，在 update 时不允许调用 hash()
        int32_t cs_sign = 0;
        if (type == SKETCHLET_CS) {
            cs_sign = (hash(&key, i + 100, 2) == 0) ? 1 : -1;
        }

        // 线性探测
#pragma unroll
        for (int probe = 0; probe < MD_MAX_PROBE; probe++) {
            uint32_t idx = (j + probe) % MD_HASH_TABLE_SIZE;

            struct MedivhBucket* bucket = bpf_map_lookup_elem(table, &idx);
            if (!bucket)
                break;

            // 进行数据竞争保护
            if (needs_lock(type)) {
                struct MedivhBucketLock* lock =
                    bpf_map_lookup_elem(locks, &idx);
                if (!lock)
                    break;

                bpf_spin_lock(&lock->lock);

                if (MD_IS_EMPTY(bucket->metadata)) {
                    bucket->metadata = MD_MAKE_METADATA(type, i);
                    bucket->original_idx = j;
                    __builtin_memset(&bucket->data, 0, sizeof(bucket->data));
                    update_mv(&bucket->data, &key);
                    bpf_spin_unlock(&lock->lock);
                    break;
                }

                if (MD_GET_ID(bucket->metadata) == i &&
                    bucket->original_idx == j) {
                    update_mv(&bucket->data, &key);
                    bpf_spin_unlock(&lock->lock);
                    break;
                }

                bpf_spin_unlock(&lock->lock);
            } else {
                if (MD_IS_EMPTY(bucket->metadata)) {
                    uint32_t new_type_id = MD_MAKE_METADATA(type, i);
                    if (__sync_bool_compare_and_swap(&bucket->metadata, 0,
                                                     new_type_id)) {
                        bucket->original_idx = j;
                        __builtin_memset(&bucket->data, 0,
                                         sizeof(bucket->data));
                        if (type == SKETCHLET_CM) {
                            update_cm(&bucket->data);
                        } else {
                            update_cs(&bucket->data, cs_sign);
                        }
                        break;
                    }
                }

                if (MD_GET_ID(bucket->metadata) == i &&
                    bucket->original_idx == j) {
                    if (type == SKETCHLET_CM) {
                        update_cm(&bucket->data);
                    } else {
                        update_cs(&bucket->data, cs_sign);
                    }
                    break;
                }
            }
        }
    }

    return XDP_PASS;
}

// GPL License
char LICENSE[] SEC("license") = "Dual BSD/GPL";
