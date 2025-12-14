#ifndef round_up
// 内核 round_up 宏
#define round_up(x, y) ((((x) + (y) - 1) / (y)) * (y))
#endif
// ebpf map 在内存中的 stride 是需要对齐的，不是按 value_type 直接摆放的
#define MMAP_STRIDE(value_type) round_up(sizeof(value_type), 8)

#ifndef UINT32_MAX
#define UINT32_MAX 0xFFFFFFFFU
#endif
#ifndef INT32_MAX
#define INT32_MAX 0x7FFFFFFF
#endif
#ifndef INT32_MIN
#define INT32_MIN ((int32_t)0x80000000)
#endif

#define CM_ROWS 4
#define CM_MEMORY (1 * 1024 * 1024)  // 1 MB
#define CM_COUNTER_TYPE uint32_t
#define CM_COLS (CM_MEMORY / CM_ROWS / sizeof(CM_COUNTER_TYPE))

#define CS_ROWS 4
#define CS_MEMORY (1 * 1024 * 1024)  // 1 MB
#define CS_COUNTER_TYPE int32_t
#define CS_COLS (CS_MEMORY / CS_ROWS / sizeof(CS_COUNTER_TYPE))

#define ES_TOTAL_MEMORY (1 * 1024 * 1024)  // 1 MB
#define ES_HEAVY_MEMORY (256 * 1024)       // 256 KB
#define ES_HEAVY_BUCKET_COUNT (ES_HEAVY_MEMORY / sizeof(struct HeavyBucket))
#define ES_LIGHT_MEMORY (ES_TOTAL_MEMORY - ES_HEAVY_MEMORY)
#define ES_LAMBDA 8      // 投票替换阈值
#define ES_LIGHT_ROWS 4  // Light Part 行数
#define ES_LIGHT_COUNTER_TYPE uint32_t
#define ES_LIGHT_COLS \
    (ES_LIGHT_MEMORY / ES_LIGHT_ROWS / sizeof(ES_LIGHT_COUNTER_TYPE))
#define ES_HEAVY_SEED 104