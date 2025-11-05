#ifndef COUNTSKETCH_H
#define COUNTSKETCH_H

#include <algorithm>
#include <memory>
#include <vector>

#include "HashFunction.h"
#include "Sketch.h"
#include "TwoTuple.h"

// uint32_t size
#define CSSKETCH_BUCKET_SIZE 4

class CountSketch : public Sketch {
   private:
    std::vector<std::vector<int>> counter_matrix;
    // 哈希行数
    uint64_t rows;
    // 哈希桶数
    uint64_t cols;
    std::unique_ptr<HashFunction> hash_function;

   public:
    CountSketch(uint64_t rows,
                uint64_t total_memory_bytes,
                std::unique_ptr<HashFunction> hash_function = nullptr);

    CountSketch(const CountSketch& other);
    CountSketch& operator=(const CountSketch& other);
    ~CountSketch() = default;

    void update(const TwoTuple& flow, int increment = 1) override;
    uint64_t query(const TwoTuple& flow) override;

    inline uint64_t get_rows() const { return rows; }
    inline uint64_t get_cols() const { return cols; }

    inline const std::vector<std::vector<int>>& get_raw_data() const {
        return counter_matrix;
    }

    inline void clear() override {
        for (auto& row : counter_matrix) {
            std::fill(row.begin(), row.end(), 0);
        }
    }
};

#endif /* COUNTSKETCH_H */