#ifndef COUNTMIN_H
#define COUNTMIN_H

#include <memory>
#include <vector>

#include "FlowKey.h"
#include "HashFunction.h"
#include "Sketch.h"

// uint32_t size
#define CMBUCKET_SIZE 4

template <typename FlowKeyType, typename SFINAE = RequireFlowKey<FlowKeyType>>
class CountMin : public Sketch<FlowKeyType> {
   private:
    std::vector<std::vector<uint32_t>> counter_matrix;
    // 哈希行数
    uint64_t rows;
    // 哈希桶数
    uint64_t cols;
    std::unique_ptr<HashFunction<FlowKeyType>> hash_function;

   public:
    CountMin(
        uint64_t rows,
        uint64_t total_memory_bytes,
        std::unique_ptr<HashFunction<FlowKeyType>> hash_function = nullptr);

    CountMin(const CountMin& other);
    CountMin& operator=(const CountMin& other);
    ~CountMin() = default;

    void update(const FlowKeyType& flow, int increment = 1) override;
    uint64_t query(const FlowKeyType& flow) override;

    inline uint64_t get_rows() const { return rows; }
    inline uint64_t get_cols() const { return cols; }

    inline const std::vector<std::vector<uint32_t>>& get_raw_data() const {
        return counter_matrix;
    }

    inline void clear() override {
        for (auto& row : counter_matrix) {
            std::fill(row.begin(), row.end(), 0);
        }
    }
};

#endif /* COUNTMIN_H */