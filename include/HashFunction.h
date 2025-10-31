#ifndef HASH_FUNCTION_H
#define HASH_FUNCTION_H

#include <cstdint>
#include <memory>

#include "../third_party/MurmurHash3.h"
#include "../third_party/SpookyV2.h"
#include "../third_party/crc32.h"
#include "TwoTuple.h"
#include "seed_list.h"

extern "C" {
#include "../third_party/crc64.h"
}

/**
 * @brief 哈希函数抽象基类
 */
class HashFunction {
   public:
    virtual ~HashFunction() = default;
    virtual std::unique_ptr<HashFunction> clone() const = 0;

    /**
     * @brief 用三方库来算 TwoTuple 的 index
     * @param flow 二元组
     * @param seed 哈希种子/索引，用于多个哈希函数
     * @param mod 最大索引值
     * @return 范围在 [0, mod) 内的哈希值
     */
    virtual uint64_t hash(const TwoTuple& flow,
                          uint64_t seed,
                          uint64_t mod) const = 0;
};

/**
 * @brief 基于 MurmurHash3 的哈希函数实现
 * 使用 MurmurHash3 算法和 seed_list 中的质数作为种子
 */
class MurmurV3HashFunction : public HashFunction {
   public:
    uint64_t hash(const TwoTuple& flow,
                  uint64_t seed,
                  uint64_t mod) const override;
    std::unique_ptr<HashFunction> clone() const override {
        return std::make_unique<MurmurV3HashFunction>();
    }
};

/**
 * @brief 基于 SpookyV2 的哈希函数实现
 * 使用 SpookyHash 算法和 seed_list 中的质数作为种子
 */
class SpookyV2HashFunction : public HashFunction {
   public:
    uint64_t hash(const TwoTuple& flow,
                  uint64_t seed,
                  uint64_t mod) const override;
    std::unique_ptr<HashFunction> clone() const override {
        return std::make_unique<SpookyV2HashFunction>();
    }
};

/**
 * @brief 基于 CRC64 的哈希函数实现
 * 使用 Redis CRC64 算法和 seed_list 中的质数作为种子
 */
class CRC64HashFunction : public HashFunction {
   public:
    uint64_t hash(const TwoTuple& flow,
                  uint64_t seed,
                  uint64_t mod) const override;
    std::unique_ptr<HashFunction> clone() const override {
        return std::make_unique<CRC64HashFunction>();
    }
};

/**
 * @brief 基于 CRC32 的哈希函数实现
 * 使用 BMv2 CRC32 算法和 seed_list 中的质数作为种子
 */
class CRC32HashFunction : public HashFunction {
   public:
    uint64_t hash(const TwoTuple& flow,
                  uint64_t seed,
                  uint64_t mod) const override;
    std::unique_ptr<HashFunction> clone() const override {
        return std::make_unique<CRC32HashFunction>();
    }
};

using DefaultHashFunction = CRC32HashFunction;

#endif  // HASH_FUNCTION_H