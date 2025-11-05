#ifndef BLOOMFILTER_H
#define BLOOMFILTER_H

#include <vector>

#include "HashFunction.h"
#include "Sketch.h"
#include "TwoTuple.h"

/**
 * @brief BloomFilter
 *
 * 用于快速判断元素是否存在，可能有假阳性但无假阴性
 */
class BloomFilter : public Sketch {
   private:
    // 位数组大小（位数）
    uint64_t num_bits;
    // 哈希函数数量
    uint64_t num_hashes;
    // 位数组
    std::vector<bool> bit_array;
    std::unique_ptr<HashFunction> hash_function;

   public:
    /**
     * @brief 构造函数
     * @param num_bits 位数组大小（位数）
     * @param num_hashes 哈希函数数量
     */
    BloomFilter(uint64_t num_bits,
                uint64_t num_hashes,
                std::unique_ptr<HashFunction> hash_function = nullptr);

    BloomFilter(const BloomFilter& other);
    BloomFilter& operator=(const BloomFilter& other);
    ~BloomFilter() = default;

    void update(const TwoTuple& flow, int increment = 1) override;
    uint64_t query(const TwoTuple& flow) override;

    inline void clear() override {
        std::fill(bit_array.begin(), bit_array.end(), 0);
    }

    inline uint64_t get_num_bits() const { return num_bits; }
    inline uint64_t get_num_hashes() const { return num_hashes; }
};

#endif /* BLOOMFILTER_H */
