#include "HashFunction.h"

template <typename FlowKeyType>
uint64_t MurmurV3HashFunction<FlowKeyType>::hash(const FlowKeyType& flow,
                                                 uint64_t seed,
                                                 uint64_t mod) const {
    uint64_t prime_seed = seed_list[seed % SEED_LIST_SIZE];

    // MurmurHash3_x64_128 produces 128 bits = 4 * 32 bits
    uint32_t result[4];

    MurmurHash3_x64_128(&flow, sizeof(FlowKeyType),
                        static_cast<uint32_t>(prime_seed), result);

    // 将128位结果转换为64位
    uint64_t hash_result = (static_cast<uint64_t>(result[0]) << 32) | result[1];

    return hash_result % mod;
}

template <typename FlowKeyType>
uint64_t SpookyV2HashFunction<FlowKeyType>::hash(const FlowKeyType& flow,
                                                 uint64_t seed,
                                                 uint64_t mod) const {
    uint64_t prime_seed = seed_list[seed % SEED_LIST_SIZE];

    uint64_t hash_result =
        SpookyHash::Hash64(&flow, sizeof(FlowKeyType), prime_seed);

    return hash_result % mod;
}

template <typename FlowKeyType>
uint64_t CRC64HashFunction<FlowKeyType>::hash(const FlowKeyType& flow,
                                              uint64_t seed,
                                              uint64_t mod) const {
    uint64_t prime_seed = seed_list[seed % SEED_LIST_SIZE];

    /*
     * 准备输入数据：FlowKey + prime_seed
     *
     * 由于 FlowKey 并不一定是 8 字节的整数倍，则必须要在 FlowKey 和 prime_seed
     * 之间显式添加 padding，否则编译器自动插入的 padding
     * 是未初始化的，会导致哈希结果不一致
     */
    struct alignas(8) InputData {
        FlowKeyType flow;
        uint8_t padding[(8 - sizeof(FlowKeyType) % 8) % 8];
        uint64_t prime_seed;

        InputData() : flow(), prime_seed(0) {
            std::memset(padding, 0, sizeof(padding));
        }
    } input;

    input.flow = flow;
    input.prime_seed = prime_seed;

    // 使用 Redis CRC64 算法计算哈希值
    uint64_t hash_result =
        crc64(0, reinterpret_cast<const unsigned char*>(&input), sizeof(input));

    return hash_result % mod;
}

template <typename FlowKeyType>
uint64_t CRC32HashFunction<FlowKeyType>::hash(const FlowKeyType& flow,
                                              uint64_t seed,
                                              uint64_t mod) const {
    uint64_t prime_seed = seed_list[seed % SEED_LIST_SIZE];

    /*
     * 准备输入数据：FlowKey + prime_seed
     *
     * 由于 FlowKey 并不一定是 8 字节的整数倍，则必须要在 FlowKey 和 prime_seed
     * 之间显式添加 padding，否则编译器自动插入的 padding
     * 是未初始化的，会导致哈希结果不一致
     */
    struct alignas(8) InputData {
        FlowKeyType flow;
        uint8_t padding[(8 - sizeof(FlowKeyType) % 8) % 8];
        uint64_t prime_seed;

        InputData() : flow(), prime_seed(0) {
            std::memset(padding, 0, sizeof(padding));
        }
    } input;

    input.flow = flow;
    input.prime_seed = prime_seed;

    // 使用 BMv2 CRC32 算法计算哈希值
    uint32_t hash_result =
        crc32(reinterpret_cast<const unsigned char*>(&input), sizeof(input));

    return static_cast<uint64_t>(hash_result) % mod;
}

// MurmurV3HashFunction
template class MurmurV3HashFunction<OneTuple>;
template class MurmurV3HashFunction<TwoTuple>;
template class MurmurV3HashFunction<FiveTuple>;

// SpookyV2HashFunction
template class SpookyV2HashFunction<OneTuple>;
template class SpookyV2HashFunction<TwoTuple>;
template class SpookyV2HashFunction<FiveTuple>;

// CRC64HashFunction
template class CRC64HashFunction<OneTuple>;
template class CRC64HashFunction<TwoTuple>;
template class CRC64HashFunction<FiveTuple>;

// CRC32HashFunction
template class CRC32HashFunction<OneTuple>;
template class CRC32HashFunction<TwoTuple>;
template class CRC32HashFunction<FiveTuple>;
