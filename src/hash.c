#include "hash.h"

uint32_t hash(const FlowKeyType* flow, uint64_t seed, uint64_t mod) {
    if (!flow)
        return 0;

    uint32_t index = (uint32_t)seed;
    if (index >= SEED_LIST_SIZE)
        return 0;

    uint64_t prime_seed = seed_list[index];

    /*
     * 准备输入数据：FlowKey + prime_seed
     *
     * 由于 FlowKey 并不一定是 8 字节的整数倍，则必须要在 FlowKey 和 prime_seed
     * 之间显式添加 padding，否则编译器自动插入的 padding
     * 是未初始化的，会导致哈希结果不一致
     */
    struct __attribute__((aligned(8))) InputData {
        FlowKeyType flow;
        uint8_t padding[(8 - sizeof(FlowKeyType) % 8) % 8];
        uint64_t prime_seed;
    } input = {};

    __builtin_memcpy(&input.flow, flow, sizeof(FlowKeyType));
    input.prime_seed = prime_seed;

    uint32_t result = crc32((const unsigned char*)&input, sizeof(input));

    if (mod)
        result %= mod;
    return result;
}