#ifndef SAMPLEANDHOLD_H
#define SAMPLEANDHOLD_H

#include <algorithm>
#include <cstdint>
#include <unordered_map>

#include "Sketch.h"
#include "TwoTuple.h"

/**
 * @brief Sample-and-Hold ，用于精确追踪重流
 *
 * 维护一个精确的流哈希表，最大容量有限。
 * 当满时，如果新流的计数更高，则驱逐最小计数器。
 */
class SampleAndHold : public Sketch {
   private:
    // 精确的二元组哈希表，如果出现哈希冲突，会由std::unordered_map处理
    using CounterMap = std::unordered_map<TwoTuple, uint64_t, TwoTupleHash>;

    // 找到计数最小的流
    CounterMap::iterator find_min();

    CounterMap counters;

    // 最多精确记录的流数
    uint64_t capacity;

   public:
    SampleAndHold(uint64_t capacity);

    void update(const TwoTuple& flow, int increment = 1) override;
    uint64_t query(const TwoTuple& flow) override;

    inline uint64_t get_capacity() const { return capacity; }
    inline uint64_t get_size() const { return counters.size(); }

    inline void clear() override { counters.clear(); }
};

#endif /* SAMPLEANDHOLD_H */
