#ifndef COUNTMIN_H
#define COUNTMIN_H

#include <memory>
#include <vector>

#include "HashFunction.h"
#include "Sketch.h"
#include "TwoTuple.h"

// uint32_t size
#define CMBUCKET_SIZE 4

class CountMin : public Sketch {
   private:
    std::vector<std::vector<uint32_t>> counter_matrix;
    // 哈希行数
    uint64_t rows;
    // 哈希桶数
    uint64_t cols;
    std::unique_ptr<HashFunction> hash_function;

   public:
    CountMin(uint64_t rows,
             uint64_t total_memory_bytes,
             std::unique_ptr<HashFunction> hash_function = nullptr);

    CountMin(const CountMin& other);
    CountMin& operator=(const CountMin& other);
    ~CountMin() = default;

    void update(const TwoTuple& flow, int increment = 1) override;
    uint64_t query(const TwoTuple& flow) override;

    inline uint64_t get_rows() const { return rows; }
    inline uint64_t get_cols() const { return cols; }

    inline void clear() {
        for (auto& row : counter_matrix) {
            std::fill(row.begin(), row.end(), 0);
        }
    }
};

#endif /* COUNTMIN_H */