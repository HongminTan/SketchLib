#include "MVSketch.h"

template <typename FlowKeyType, typename SFINAE>
MVSketch<FlowKeyType, SFINAE>::MVSketch(
    uint64_t rows,
    uint64_t total_memory_bytes,
    std::unique_ptr<HashFunction<FlowKeyType>> hash_function)
    : rows(rows), hash_function(std::move(hash_function)) {
    if (!this->hash_function) {
        this->hash_function =
            std::make_unique<DefaultHashFunction<FlowKeyType>>();
    }
    cols = total_memory_bytes / rows / sizeof(MVBucket<FlowKeyType>);
    matrix = std::vector<std::vector<MVBucket<FlowKeyType>>>(
        rows, std::vector<MVBucket<FlowKeyType>>(cols));
}

template <typename FlowKeyType, typename SFINAE>
MVSketch<FlowKeyType, SFINAE>::MVSketch(const MVSketch& other)
    : matrix(other.matrix),
      rows(other.rows),
      cols(other.cols),
      hash_function(
          other.hash_function
              ? other.hash_function->clone()
              : std::make_unique<DefaultHashFunction<FlowKeyType>>()) {}

template <typename FlowKeyType, typename SFINAE>
MVSketch<FlowKeyType, SFINAE>& MVSketch<FlowKeyType, SFINAE>::operator=(
    const MVSketch& other) {
    if (this != &other) {
        matrix = other.matrix;
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
void MVSketch<FlowKeyType, SFINAE>::update(const FlowKeyType& flow,
                                           int increment) {
    if (increment <= 0) {
        return;
    }

    for (uint64_t hash_index = 0; hash_index < rows; hash_index++) {
        uint64_t bucket_index = hash_function->hash(flow, hash_index, cols);

        // 更新总计数
        uint64_t new_value =
            static_cast<uint64_t>(matrix[hash_index][bucket_index].value) +
            increment;
        if (new_value > UINT32_MAX) {
            new_value = UINT32_MAX;
        }
        matrix[hash_index][bucket_index].value =
            static_cast<uint32_t>(new_value);

        if (matrix[hash_index][bucket_index].flow_id == flow) {
            // 更新流是候选流，增加计数
            int64_t new_count =
                static_cast<int64_t>(matrix[hash_index][bucket_index].count) +
                increment;
            if (new_count > INT32_MAX) {
                new_count = INT32_MAX;
            }
            matrix[hash_index][bucket_index].count =
                static_cast<int32_t>(new_count);
        } else {
            // 更新流不是候选流，减少计数
            int64_t new_count =
                static_cast<int64_t>(matrix[hash_index][bucket_index].count) -
                increment;
            if (new_count < INT32_MIN) {
                new_count = INT32_MIN;
            }
            matrix[hash_index][bucket_index].count =
                static_cast<int32_t>(new_count);

            // 如果 count < 0，说明更新流更占优，替换候选流
            if (matrix[hash_index][bucket_index].count < 0) {
                matrix[hash_index][bucket_index].flow_id = flow;
                matrix[hash_index][bucket_index].count *= -1;
            }
        }
    }
}

template <typename FlowKeyType, typename SFINAE>
uint64_t MVSketch<FlowKeyType, SFINAE>::query(const FlowKeyType& flow) const {
    uint64_t min_estimate = UINT64_MAX;

    for (uint64_t hash_index = 0; hash_index < rows; hash_index++) {
        uint64_t bucket_index = hash_function->hash(flow, hash_index, cols);

        uint64_t estimate;
        if (matrix[hash_index][bucket_index].flow_id == flow) {
            // 桶中存储的就是查询流
            // 估计值 = (value + count) / 2
            uint64_t value = matrix[hash_index][bucket_index].value;
            int64_t count = matrix[hash_index][bucket_index].count;
            estimate = (value + static_cast<uint64_t>(count)) / 2;
        } else {
            // 桶中存储的不是查询流
            // 估计值 = (value - count) / 2
            uint64_t value = matrix[hash_index][bucket_index].value;
            int64_t count = matrix[hash_index][bucket_index].count;
            // 确保 value >= count，避免下溢
            if (value >= static_cast<uint64_t>(count)) {
                estimate = (value - static_cast<uint64_t>(count)) / 2;
            } else {
                estimate = 0;
            }
        }

        min_estimate = std::min(min_estimate, estimate);
    }

    return min_estimate;
}

// 模板实例化
template class MVSketch<OneTuple>;
template class MVSketch<TwoTuple>;
template class MVSketch<FiveTuple>;
