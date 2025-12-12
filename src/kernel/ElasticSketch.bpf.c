// 开启 BPF 编译模式
#define __BPF__ 1

#include "ElasticSketch.h"
#include "Config.h"
#include "FlowKey.h"
#include "autogen/vmlinux.h"
#include "hash.h"

#include <bpf/bpf_helpers.h>

// Heavy Part Maps
struct heavy_buckets {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(max_entries, ES_HEAVY_BUCKET_COUNT);
    __uint(map_flags, BPF_F_MMAPABLE);
    __type(key, uint32_t);
    __type(value, struct HeavyBucket);
};

// Heavy Part double buffer
struct heavy_buckets heavy_part_0 SEC(".maps");
struct heavy_buckets heavy_part_1 SEC(".maps");

// Heavy Part buffer 选择 map
struct {
    __uint(type, BPF_MAP_TYPE_ARRAY_OF_MAPS);
    __uint(max_entries, 1);
    __type(key, uint32_t);
    __array(values, struct heavy_buckets);
} select_heavy_part SEC(".maps") = {
    .values = {&heavy_part_0},
};

// Light Part Maps
struct light_counters {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(max_entries, ES_LIGHT_ROWS* ES_LIGHT_COLS);
    __uint(map_flags, BPF_F_MMAPABLE);
    __type(key, uint32_t);
    __type(value, ES_LIGHT_COUNTER_TYPE);
};

// Light Part double buffer
struct light_counters light_counts_0 SEC(".maps");
struct light_counters light_counts_1 SEC(".maps");

// Light Part buffer 选择 map
struct {
    __uint(type, BPF_MAP_TYPE_ARRAY_OF_MAPS);
    __uint(max_entries, 1);
    __type(key, uint32_t);
    __array(values, struct light_counters);
} select_light_count SEC(".maps") = {
    .values = {&light_counts_0},
};

// 更新 Light Part
static __always_inline void update_light_part(void* light_part,
                                              const FlowKeyType* flow,
                                              uint32_t increment) {
#pragma unroll
    for (int row = 0; row < ES_LIGHT_ROWS; ++row) {
        uint32_t index = hash(flow, row, ES_LIGHT_COLS);
        uint32_t offset = (uint32_t)(row * ES_LIGHT_COLS + index);

        ES_LIGHT_COUNTER_TYPE* counter =
            bpf_map_lookup_elem(light_part, &offset);
        if (counter) {
            __sync_fetch_and_add(counter, increment);
        }
    }
}

SEC("xdp")
int update(struct xdp_md* ctx) {
    FlowKeyType key;
    if (extract_flowkey(ctx, &key))
        return XDP_PASS;

    // 获取当前使用的 Heavy Part
    uint32_t zero = 0;
    void* heavy_part = bpf_map_lookup_elem(&select_heavy_part, &zero);
    if (!heavy_part)
        return XDP_PASS;

    // 获取当前使用的 Light Part
    void* light_part = bpf_map_lookup_elem(&select_light_count, &zero);
    if (!light_part)
        return XDP_PASS;

    // 计算 Heavy Part 桶索引
    uint32_t bucket_idx = hash(&key, ES_HEAVY_SEED, ES_HEAVY_BUCKET_COUNT);

    struct HeavyBucket* bucket = bpf_map_lookup_elem(heavy_part, &bucket_idx);
    if (!bucket)
        return XDP_PASS;

    FlowKeyType empty_flow = {};
    // 桶为空，直接插入
    if (__builtin_memcmp(&bucket->flow_id, &empty_flow, sizeof(FlowKeyType)) ==
        0) {
        __builtin_memcpy(&bucket->flow_id, &key, sizeof(FlowKeyType));
        bucket->pos_vote = 1;
        bucket->neg_vote = 0;
        bucket->flag = 0;
        return XDP_PASS;
    }

    // 是同一个流，增加正票
    if (__builtin_memcmp(&bucket->flow_id, &key, sizeof(FlowKeyType)) == 0) {
        __sync_fetch_and_add(&bucket->pos_vote, 1);
        return XDP_PASS;
    }

    // 是不同的流，增加负票
    __sync_fetch_and_add(&bucket->neg_vote, 1);

    if (bucket->pos_vote > 0 &&
        bucket->neg_vote >= ES_LAMBDA * bucket->pos_vote) {
        // 旧流被踢出
        FlowKeyType evicted_flow;
        __builtin_memcpy(&evicted_flow, &bucket->flow_id, sizeof(FlowKeyType));
        uint32_t evicted_count = bucket->pos_vote;

        // 标记发生过替换
        bucket->flag = 1;

        // 替换为新流
        __builtin_memcpy(&bucket->flow_id, &key, sizeof(FlowKeyType));
        bucket->pos_vote = 1;
        bucket->neg_vote = 0;

        // 将被踢出的流更新到 Light Part
        update_light_part(light_part, &evicted_flow, evicted_count);
    } else {
        // 不替换，将当前流更新到 Light Part
        update_light_part(light_part, &key, 1);
    }

    return XDP_PASS;
}

// GPL License
char LICENSE[] SEC("license") = "Dual BSD/GPL";
