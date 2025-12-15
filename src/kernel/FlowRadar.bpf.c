// 开启 BPF 编译模式
#define __BPF__ 1

#include "FlowRadar.h"
#include "Config.h"
#include "FlowKey.h"
#include "autogen/vmlinux.h"
#include "hash.h"

#include <bpf/bpf_helpers.h>

// BloomFilter Maps
struct bf_array {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(max_entries, FR_BF_NUM_WORDS);
    __uint(map_flags, BPF_F_MMAPABLE);
    __type(key, uint32_t);
    __type(value, uint32_t);
};

// BloomFilter double buffer
struct bf_array bf_0 SEC(".maps");
struct bf_array bf_1 SEC(".maps");

// BloomFilter buffer 选择 map
struct {
    __uint(type, BPF_MAP_TYPE_ARRAY_OF_MAPS);
    __uint(max_entries, 1);
    __type(key, uint32_t);
    __array(values, struct bf_array);
} select_bf SEC(".maps") = {
    .values = {&bf_0},
};

// CountingTable 数据 Maps
struct ct_buckets {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(max_entries, FR_CT_SIZE);
    __uint(map_flags, BPF_F_MMAPABLE);
    __type(key, uint32_t);
    __type(value, struct FRBucket);
};

// CountingTable 数据 double buffer
struct ct_buckets ct_0 SEC(".maps");
struct ct_buckets ct_1 SEC(".maps");

// CountingTable 数据 buffer 选择 map
struct {
    __uint(type, BPF_MAP_TYPE_ARRAY_OF_MAPS);
    __uint(max_entries, 1);
    __type(key, uint32_t);
    __array(values, struct ct_buckets);
} select_ct SEC(".maps") = {
    .values = {&ct_0},
};

// CountingTable 锁 Maps
struct ct_locks {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(max_entries, FR_CT_SIZE);
    __type(key, uint32_t);
    __type(value, struct FRBucketLock);
};

// CountingTable 锁 double buffer
struct ct_locks ct_locks_0 SEC(".maps");
struct ct_locks ct_locks_1 SEC(".maps");

// CountingTable 锁 buffer 选择 map
struct {
    __uint(type, BPF_MAP_TYPE_ARRAY_OF_MAPS);
    __uint(max_entries, 1);
    __type(key, uint32_t);
    __array(values, struct ct_locks);
} select_ct_lock SEC(".maps") = {
    .values = {&ct_locks_0},
};

// 原子查询并插入 BloomFilter
// 返回值: 1 表示之前已存在，0 表示首次出现
static __always_inline int bf_query_and_update(void* bf,
                                               const FlowKeyType* flow) {
    int result = 1;

#pragma unroll
    for (int i = 0; i < FR_BF_NUM_HASHES; ++i) {
        // 计算位数组索引
        uint32_t bit_index = hash(flow, i, FR_BF_NUM_BITS);
        uint32_t word_index = bit_index / FR_BITS_PER_WORD;
        uint32_t bit_mask = 1U << (bit_index % FR_BITS_PER_WORD);

        uint32_t* word = bpf_map_lookup_elem(bf, &word_index);
        if (!word) {
            result = 0;
            continue;
        }

        // 原子设置位并获取旧值
        uint32_t old_val = __sync_fetch_and_or(word, bit_mask);

        // 检查该位之前是否已设置
        if (!(old_val & bit_mask)) {
            result = 0;
        }
    }

    return result;
}

// XOR FlowKey
static __always_inline void xor_flowkey(FlowKeyType* dst,
                                        const FlowKeyType* src) {
    uint8_t* d = (uint8_t*)dst;
    const uint8_t* s = (const uint8_t*)src;
#pragma unroll
    for (int i = 0; i < (int)sizeof(FlowKeyType); ++i) {
        d[i] ^= s[i];
    }
}

SEC("xdp")
int update(struct xdp_md* ctx) {
    FlowKeyType key;
    if (extract_flowkey(ctx, &key))
        return XDP_PASS;

    // 获取当前使用的 BloomFilter
    uint32_t zero = 0;
    void* bf = bpf_map_lookup_elem(&select_bf, &zero);
    if (!bf)
        return XDP_PASS;

    // 获取当前使用的 CountingTable
    void* ct = bpf_map_lookup_elem(&select_ct, &zero);
    if (!ct)
        return XDP_PASS;

    // 获取当前使用的 CountingTable 锁
    void* ct_lock = bpf_map_lookup_elem(&select_ct_lock, &zero);
    if (!ct_lock)
        return XDP_PASS;

    // 原子查询并插入 BloomFilter
    int exists = bf_query_and_update(bf, &key);

    // 更新 CountingTable
#pragma unroll
    for (int i = 0; i < FR_CT_NUM_HASHES; ++i) {
        uint32_t index = hash(&key, i, FR_CT_SIZE);

        struct FRBucket* bucket = bpf_map_lookup_elem(ct, &index);
        if (!bucket)
            continue;

        struct FRBucketLock* lock = bpf_map_lookup_elem(ct_lock, &index);
        if (!lock)
            continue;

        bpf_spin_lock(&lock->lock);

        if (!exists) {
            // 首次出现：XOR 流ID，流数+1
            xor_flowkey(&bucket->flow_xor, &key);
            if (bucket->flow_count < UINT32_MAX) {
                bucket->flow_count++;
            }
        }

        // 包数+1
        if (bucket->packet_count < UINT32_MAX) {
            bucket->packet_count++;
        }

        bpf_spin_unlock(&lock->lock);
    }

    return XDP_PASS;
}

// GPL License
char LICENSE[] SEC("license") = "Dual BSD/GPL";
