#ifndef MVSKETCH_H
#define MVSKETCH_H

#include <memory>
#include <vector>

#include "FlowKey.h"
#include "HashFunction.h"
#include "Sketch.h"

/**
 * @brief MajorityVote Sketch 的桶结构
 *
 * 每个桶存储一个候选流及其相关信息：
 * - value: 所有映射到该桶的流的总计数
 * - flow_id: 当前存储的候选流标识符
 * - count: 候选流的净计数
 */
template <typename FlowKeyType>
struct MVBucket {
    // 当前存储的候选流标识符
    FlowKeyType flow_id;
    // 所有映射到该桶的流的总计数
    uint32_t value;
    // 当前候选流的净计数
    int32_t count;

    MVBucket() : flow_id(), value(0), count(0) {}

    inline bool empty() const { return flow_id == FlowKeyType(); }

    inline void clear() {
        flow_id = FlowKeyType();
        value = 0;
        count = 0;
    }
};

/**
 * @brief Majority Vote Sketch
 *
 * 使用投票机制识别主要流的频率估计算法
 * 每个桶存储一个候选流，通过 count 的正负来识别主要流
 */
template <typename FlowKeyType, typename SFINAE = RequireFlowKey<FlowKeyType>>
class MVSketch : public Sketch<FlowKeyType> {
   private:
    std::vector<std::vector<MVBucket<FlowKeyType>>> matrix;
    // 哈希行数
    uint64_t rows;
    // 哈希桶数
    uint64_t cols;
    std::unique_ptr<HashFunction<FlowKeyType>> hash_function;

   public:
    MVSketch(
        uint64_t rows,
        uint64_t total_memory_bytes,
        std::unique_ptr<HashFunction<FlowKeyType>> hash_function = nullptr);

    MVSketch(const MVSketch& other);
    MVSketch& operator=(const MVSketch& other);
    ~MVSketch() = default;

    void update(const FlowKeyType& flow, int increment = 1) override;
    uint64_t query(const FlowKeyType& flow) const override;

    inline uint64_t get_rows() const { return rows; }
    inline uint64_t get_cols() const { return cols; }

    inline void clear() override {
        for (auto& row : matrix) {
            for (auto& bucket : row) {
                bucket.clear();
            }
        }
    }
};

#endif /* MVSKETCH_H */
