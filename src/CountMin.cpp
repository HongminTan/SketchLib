#include "CountMin.h"

template <typename FlowKeyType, typename SFINAE>
CountMin<FlowKeyType, SFINAE>::CountMin(
    uint64_t rows,
    uint64_t total_memory_bytes,
    std::unique_ptr<HashFunction<FlowKeyType>> hash_function)
    : rows(rows), hash_function(std::move(hash_function)) {
    if (!this->hash_function) {
        this->hash_function =
            std::make_unique<DefaultHashFunction<FlowKeyType>>();
    }
    cols = total_memory_bytes / rows / CMBUCKET_SIZE;
    counter_matrix = std::vector<std::vector<uint32_t>>(
        rows, std::vector<uint32_t>(cols, 0));
}

template <typename FlowKeyType, typename SFINAE>
CountMin<FlowKeyType, SFINAE>::CountMin(const CountMin& other)
    : counter_matrix(other.counter_matrix),
      rows(other.rows),
      cols(other.cols),
      hash_function(
          other.hash_function
              ? other.hash_function->clone()
              : std::make_unique<DefaultHashFunction<FlowKeyType>>()) {}

template <typename FlowKeyType, typename SFINAE>
CountMin<FlowKeyType, SFINAE>& CountMin<FlowKeyType, SFINAE>::operator=(
    const CountMin& other) {
    if (this != &other) {
        counter_matrix = other.counter_matrix;
        rows = other.rows;
        cols = other.cols;
        hash_function =
            other.hash_function
                ? other.hash_function->clone()
                : std::make_unique<DefaultHashFunction<FlowKeyType>>();
    }
    return *this;
}

template <typename FlowKeyType, typename SFINAE>
void CountMin<FlowKeyType, SFINAE>::update(const FlowKeyType& flow,
                                           int increment) {
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

template <typename FlowKeyType, typename SFINAE>
uint64_t CountMin<FlowKeyType, SFINAE>::query(const FlowKeyType& flow) {
    uint32_t min_count = 0xffffffff;
    for (uint64_t hash_index = 0; hash_index < rows; hash_index++) {
        uint64_t bucket_index = hash_function->hash(flow, hash_index, cols);
        min_count =
            std::min(min_count, counter_matrix[hash_index][bucket_index]);
    }
    return min_count;
}

template class CountMin<OneTuple>;
template class CountMin<TwoTuple>;
template class CountMin<FiveTuple>;