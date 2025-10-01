#include "CountSketch.h"

CountSketch::CountSketch(uint64_t rows,
                         uint64_t total_memory_bytes,
                         std::unique_ptr<HashFunction> hash_function)
    : rows(rows), hash_function(std::move(hash_function)) {
    if (!this->hash_function) {
        this->hash_function = std::make_unique<DefaultHashFunction>();
    }
    cols = total_memory_bytes / rows / CSSKETCH_BUCKET_SIZE;
    counter_matrix =
        std::vector<std::vector<int>>(rows, std::vector<int>(cols, 0));
}

CountSketch::CountSketch(const CountSketch& other)
    : counter_matrix(other.counter_matrix),
      rows(other.rows),
      cols(other.cols),
      hash_function(other.hash_function
                        ? other.hash_function->clone()
                        : std::make_unique<DefaultHashFunction>()) {}

CountSketch& CountSketch::operator=(const CountSketch& other) {
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

void CountSketch::update(const TwoTuple& flow, int increment) {
    for (uint64_t hash_index = 0; hash_index < rows; hash_index++) {
        uint64_t bucket_index = hash_function->hash(flow, hash_index, cols);
        uint64_t sign_hash = hash_function->hash(flow, hash_index + rows, 2);
        counter_matrix[hash_index][bucket_index] +=
            sign_hash ? increment : -increment;
    }
}

uint64_t CountSketch::query(const TwoTuple& flow) {
    std::vector<int64_t> estimates(rows);
    for (uint64_t hash_index = 0; hash_index < rows; hash_index++) {
        uint64_t bucket_index = hash_function->hash(flow, hash_index, cols);
        uint64_t sign_hash = hash_function->hash(flow, hash_index + rows, 2);
        estimates[hash_index] = sign_hash
                                    ? counter_matrix[hash_index][bucket_index]
                                    : -counter_matrix[hash_index][bucket_index];
    }
    // 取中位数
    std::sort(estimates.begin(), estimates.end());
    uint64_t median_index = rows / 2;
    int64_t median_value;
    if (rows % 2 == 0) {
        median_value =
            (estimates[median_index] + estimates[median_index - 1]) / 2;
    } else {
        median_value = estimates[median_index];
    }
    return static_cast<uint64_t>(std::max(0LL, median_value));
}