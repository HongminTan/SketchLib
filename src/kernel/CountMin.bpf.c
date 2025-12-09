// 开启 BPF 编译模式
#define __BPF__ 1

#include "Config.h"
#include "FlowKey.h"
#include "autogen/vmlinux.h"
#include "hash.h"

#include <bpf/bpf_helpers.h>

// Counter map with mmap support
struct counters {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(max_entries, CM_ROWS* CM_COLS);
    __uint(map_flags, BPF_F_MMAPABLE);
    __type(key, uint32_t);
    __type(value, uint32_t);
};

// counter maps double buffer
struct counters counters_0 SEC(".maps");
struct counters counters_1 SEC(".maps");

// 传递 buffer 选择 ARRAY_OF_MAPS
struct {
    __uint(type, BPF_MAP_TYPE_ARRAY_OF_MAPS);
    __uint(max_entries, 1);
    __type(key, uint32_t);
    __array(values, struct counters);
} select_counter SEC(".maps") = {
    .values = {&counters_0},
};

SEC("xdp")
int update(struct xdp_md* ctx) {
    FlowKeyType key;
    if (extract_flowkey(ctx, &key))
        return XDP_PASS;

    // 获取当前使用的 counter map
    uint32_t zero = 0;
    void* current_counters = bpf_map_lookup_elem(&select_counter, &zero);
    if (!current_counters)
        return XDP_PASS;

#pragma unroll
    for (int row = 0; row < CM_ROWS; ++row) {
        // 计算索引
        uint32_t index = hash(&key, row, CM_COLS);
        uint32_t offset = (uint32_t)(row * CM_COLS + index);

        uint32_t* counter = bpf_map_lookup_elem(current_counters, &offset);
        if (counter) {
            __sync_fetch_and_add(counter, 1);
        }
    }

    return XDP_PASS;
}

// GPL License
char LICENSE[] SEC("license") = "Dual BSD/GPL";