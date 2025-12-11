#ifndef round_up
// 内核 round_up 宏
#define round_up(x, y) ((((x) + (y) - 1) / (y)) * (y))
#endif
// ebpf map 在内存中的 stride 是需要对齐的，不是按 value_type 直接摆放的
#define MMAP_STRIDE(value_type) round_up(sizeof(value_type), 8)

#define CM_ROWS 4
#define CM_MEMORY (1 * 1024 * 1024)  // 1 MB
#define CM_COUNTER_TYPE uint32_t
#define CM_COLS (CM_MEMORY / CM_ROWS / sizeof(CM_COUNTER_TYPE))

#define CS_ROWS 4
#define CS_MEMORY (1 * 1024 * 1024)  // 1 MB
#define CS_COUNTER_TYPE int32_t
#define CS_COLS (CS_MEMORY / CS_ROWS / sizeof(CS_COUNTER_TYPE))