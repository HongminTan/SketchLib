#include "BloomFilter.h"

BloomFilter::BloomFilter(uint64_t num_bits,
                         uint64_t num_hashes,
                         std::unique_ptr<HashFunction> hash_function)
    : num_bits(num_bits),
      num_hashes(num_hashes),
      bit_array(num_bits, false),
      hash_function(std::move(hash_function)) {
    if (!this->hash_function) {
        this->hash_function = std::make_unique<DefaultHashFunction>();
    }
}

BloomFilter::BloomFilter(const BloomFilter& other)
    : num_bits(other.num_bits),
      num_hashes(other.num_hashes),
      bit_array(other.bit_array),
      hash_function(other.hash_function
                        ? other.hash_function->clone()
                        : std::make_unique<DefaultHashFunction>()) {}

BloomFilter& BloomFilter::operator=(const BloomFilter& other) {
    if (this != &other) {
        num_bits = other.num_bits;
        num_hashes = other.num_hashes;
        bit_array = other.bit_array;
        hash_function = other.hash_function
                            ? other.hash_function->clone()
                            : std::make_unique<DefaultHashFunction>();
    }
    return *this;
}

void BloomFilter::update(const TwoTuple& flow, int increment) {
    // BloomFilter 无需 increment
    (void)increment;

    // 使用多个哈希函数设置对应位
    for (uint64_t i = 0; i < num_hashes; i++) {
        uint64_t index = hash_function->hash(flow, i, num_bits);
        bit_array[index] = true;
    }
}

uint64_t BloomFilter::query(const TwoTuple& flow) {
    // 检查所有哈希位置，全部为 true 才返回存在
    for (uint64_t i = 0; i < num_hashes; i++) {
        uint64_t index = hash_function->hash(flow, i, num_bits);
        if (!bit_array[index]) {
            // 真阴性
            return 0;
        }
    }
    // 可能存在
    return 1;
}
