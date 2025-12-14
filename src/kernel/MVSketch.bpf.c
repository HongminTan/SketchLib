// 开启 BPF 编译模式
#define __BPF__ 1

#include "MVSketch.h"
#include "Config.h"
#include "FlowKey.h"
#include "autogen/vmlinux.h"
#include "hash.h"

#include <bpf/bpf_helpers.h>

// MVSketch 数据 Maps
struct mv_buckets {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(max_entries, MV_ROWS* MV_COLS);
    __uint(map_flags, BPF_F_MMAPABLE);
    __type(key, uint32_t);
    __type(value, struct MVBucket);
};

// 数据 double buffer
struct mv_buckets buckets_0 SEC(".maps");
struct mv_buckets buckets_1 SEC(".maps");

// 数据 buffer 选择 map
struct {
    __uint(type, BPF_MAP_TYPE_ARRAY_OF_MAPS);
    __uint(max_entries, 1);
    __type(key, uint32_t);
    __array(values, struct mv_buckets);
} select_counter SEC(".maps") = {
    .values = {&buckets_0},
};

// MVSketch 锁 Maps
struct mv_locks {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(max_entries, MV_ROWS* MV_COLS);
    __type(key, uint32_t);
    __type(value, struct MVBucketLock);
};

// 锁 double buffer
struct mv_locks locks_0 SEC(".maps");
struct mv_locks locks_1 SEC(".maps");

// 锁 buffer 选择 map
struct {
    __uint(type, BPF_MAP_TYPE_ARRAY_OF_MAPS);
    __uint(max_entries, 1);
    __type(key, uint32_t);
    __array(values, struct mv_locks);
} select_lock SEC(".maps") = {
    .values = {&locks_0},
};

SEC("xdp")
int update(struct xdp_md* ctx) {
    FlowKeyType key;
    if (extract_flowkey(ctx, &key))
        return XDP_PASS;

    // 获取当前使用的数据 map
    uint32_t zero = 0;
    void* current_buckets = bpf_map_lookup_elem(&select_counter, &zero);
    if (!current_buckets)
        return XDP_PASS;

    // 获取当前使用的锁 map
    void* lock = bpf_map_lookup_elem(&select_lock, &zero);
    if (!lock)
        return XDP_PASS;

#pragma unroll
    for (int row = 0; row < MV_ROWS; ++row) {
        // 计算桶索引
        uint32_t index = hash(&key, row, MV_COLS);
        uint32_t offset = (uint32_t)(row * MV_COLS + index);

        struct MVBucket* bucket = bpf_map_lookup_elem(current_buckets, &offset);
        if (!bucket)
            continue;

        struct MVBucketLock* bucket_lock = bpf_map_lookup_elem(lock, &offset);
        if (!bucket_lock)
            continue;

        // 加锁保护桶操作
        bpf_spin_lock(&bucket_lock->lock);

        // 更新总计数 value
        if (bucket->value < UINT32_MAX) {
            bucket->value++;
        }

        if (__builtin_memcmp(&bucket->flow_id, &key, sizeof(FlowKeyType)) ==
            0) {
            // 更新流是候选流，增加计数
            if (bucket->count < INT32_MAX) {
                bucket->count++;
            }
        } else {
            // 更新流不是候选流，减少计数
            if (bucket->count > INT32_MIN) {
                bucket->count--;
            }

            // 如果 count < 0，说明更新流更占优，替换候选流
            if (bucket->count < 0) {
                __builtin_memcpy(&bucket->flow_id, &key, sizeof(FlowKeyType));
                bucket->count = -bucket->count;
            }
        }

        bpf_spin_unlock(&bucket_lock->lock);
    }

    return XDP_PASS;
}

// GPL License
char LICENSE[] SEC("license") = "Dual BSD/GPL";
