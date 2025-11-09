#ifndef HASH_FUNCTION_H
#define HASH_FUNCTION_H

#include <cstdint>
#include <cstring>
#include <memory>

#include "../third_party/MurmurHash3.h"
#include "../third_party/SpookyV2.h"
#include "../third_party/crc32.h"
#include "FlowKey.h"
#include "seed_list.h"

extern "C" {
#include "../third_party/crc64.h"
}

/**
 * @brief 哈希函数抽象模板基类
 *
 * @tparam FlowKeyType 流标识符类型（OneTuple, TwoTuple, FiveTuple）
 */
template <typename FlowKeyType>
class HashFunction {
   public:
    virtual ~HashFunction() = default;
    virtual std::unique_ptr<HashFunction<FlowKeyType>> clone() const = 0;

    /**
     * @brief 计算 FlowKey 的哈希值
     * @param flow 流标识符
     * @param seed 哈希种子/索引，用于多个哈希函数
     * @param mod 最大索引值
     * @return 范围在 [0, mod) 内的哈希值
     */
    virtual uint64_t hash(const FlowKeyType& flow,
                          uint64_t seed,
                          uint64_t mod) const = 0;
};

/**
 * @brief 基于 MurmurHash3 的哈希函数实现
 * 使用 MurmurHash3 算法和 seed_list 中的质数作为种子
 */
template <typename FlowKeyType>
class MurmurV3HashFunction : public HashFunction<FlowKeyType> {
   public:
    uint64_t hash(const FlowKeyType& flow,
                  uint64_t seed,
                  uint64_t mod) const override;
    std::unique_ptr<HashFunction<FlowKeyType>> clone() const override {
        return std::make_unique<MurmurV3HashFunction<FlowKeyType>>();
    }
};

/**
 * @brief 基于 SpookyV2 的哈希函数实现
 * 使用 SpookyHash 算法和 seed_list 中的质数作为种子
 */
template <typename FlowKeyType>
class SpookyV2HashFunction : public HashFunction<FlowKeyType> {
   public:
    uint64_t hash(const FlowKeyType& flow,
                  uint64_t seed,
                  uint64_t mod) const override;
    std::unique_ptr<HashFunction<FlowKeyType>> clone() const override {
        return std::make_unique<SpookyV2HashFunction<FlowKeyType>>();
    }
};

/**
 * @brief 基于 CRC64 的哈希函数实现
 * 使用 Redis CRC64 算法和 seed_list 中的质数作为种子
 */
template <typename FlowKeyType>
class CRC64HashFunction : public HashFunction<FlowKeyType> {
   public:
    uint64_t hash(const FlowKeyType& flow,
                  uint64_t seed,
                  uint64_t mod) const override;
    std::unique_ptr<HashFunction<FlowKeyType>> clone() const override {
        return std::make_unique<CRC64HashFunction<FlowKeyType>>();
    }
};

/**
 * @brief 基于 CRC32 的哈希函数实现
 * 使用 BMv2 CRC32 算法和 seed_list 中的质数作为种子
 */
template <typename FlowKeyType>
class CRC32HashFunction : public HashFunction<FlowKeyType> {
   public:
    uint64_t hash(const FlowKeyType& flow,
                  uint64_t seed,
                  uint64_t mod) const override;
    std::unique_ptr<HashFunction<FlowKeyType>> clone() const override {
        return std::make_unique<CRC32HashFunction<FlowKeyType>>();
    }
};

// 默认哈希函数类型别名
template <typename FlowKeyType>
using DefaultHashFunction = CRC32HashFunction<FlowKeyType>;

#endif  // HASH_FUNCTION_H