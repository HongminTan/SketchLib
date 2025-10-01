#include "HashFunction.h"

uint64_t MurmurV3HashFunction::hash(const TwoTuple& flow,
                                    uint64_t seed,
                                    uint64_t mod) const {
    uint64_t prime_seed = seed_list[seed % SEED_LIST_SIZE];

    // 准备内存
    struct {
        uint32_t src_ip;
        uint32_t dst_ip;
        uint64_t prime_seed;
    } input = {flow.src_ip, flow.dst_ip, prime_seed};

    // MurmurHash3_x86_128 produces 128 bits = 4 * 32 bits
    uint32_t result[4];

    MurmurHash3_x64_128(&input, sizeof(input),
                        static_cast<uint32_t>(prime_seed), result);

    // 将128位结果转换为64位
    uint64_t hash_result = (static_cast<uint64_t>(result[0]) << 32) | result[1];

    return hash_result % mod;
}

uint64_t SpookyV2HashFunction::hash(const TwoTuple& flow,
                                    uint64_t seed,
                                    uint64_t mod) const {
    uint64_t prime_seed = seed_list[seed % SEED_LIST_SIZE];

    // 准备内存
    struct {
        uint32_t src_ip;
        uint32_t dst_ip;
        uint64_t prime_seed;
    } input = {flow.src_ip, flow.dst_ip, prime_seed};

    uint64_t hash_result =
        SpookyHash::Hash64(&input, sizeof(input), prime_seed);

    return hash_result % mod;
}
