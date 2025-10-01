#include "CountMin.h"

CountMin::CountMin(uint64_t rows,
                   uint64_t total_memory_bytes,
                   std::unique_ptr<HashFunction> hash_function)
    : rows(rows), hash_function(std::move(hash_function)) {
    if (!this->hash_function) {
        this->hash_function = std::make_unique<DefaultHashFunction>();
    }
    cols = total_memory_bytes / rows / CMBUCKET_SIZE;
    counter_matrix = std::vector<std::vector<uint32_t>>(
        rows, std::vector<uint32_t>(cols, 0));
}

CountMin::CountMin(const CountMin& other)
    : counter_matrix(other.counter_matrix),
      rows(other.rows),
      cols(other.cols),
      hash_function(other.hash_function
                        ? other.hash_function->clone()
                        : std::make_unique<DefaultHashFunction>()) {}

CountMin& CountMin::operator=(const CountMin& other) {
    if (this != &other) {
        counter_matrix = other.counter_matrix;
        rows = other.rows;
        cols = other.cols;
        hash_function = other.hash_function
                            ? other.hash_function->clone()
                            : std::make_unique<DefaultHashFunction>();
    }
    return *this;
}

void CountMin::update(const TwoTuple& flow, int increment) {
    for (uint64_t hash_index = 0; hash_index < rows; hash_index++) {
        uint64_t bucket_index = hash_function->hash(flow, hash_index, cols);
        uint64_t new_value =
            static_cast<uint64_t>(counter_matrix[hash_index][bucket_index]) +
            increment;
        if (new_value >= 0xffffffff) {
            new_value = 0xffffffff;
        }
        counter_matrix[hash_index][bucket_index] =
            static_cast<uint32_t>(new_value);
    }
}

uint64_t CountMin::query(const TwoTuple& flow) {
    uint32_t min_count = 0xffffffff;
    for (uint64_t hash_index = 0; hash_index < rows; hash_index++) {
        uint64_t bucket_index = hash_function->hash(flow, hash_index, cols);
        min_count =
            std::min(min_count, counter_matrix[hash_index][bucket_index]);
    }
    return min_count;
}