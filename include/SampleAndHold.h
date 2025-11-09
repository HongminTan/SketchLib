#ifndef SAMPLEANDHOLD_H
#define SAMPLEANDHOLD_H

#include <algorithm>
#include <cstdint>
#include <unordered_map>

#include "FlowKey.h"
#include "Sketch.h"

/**
 * @brief Sample-and-Hold ，用于精确追踪重流
 *
 * 维护一个精确的流哈希表，最大容量有限。
 * 当满时，如果新流的计数更高，则驱逐最小计数器。
 */
template <typename FlowKeyType, typename SFINAE = RequireFlowKey<FlowKeyType>>
class SampleAndHold : public Sketch<FlowKeyType> {
   private:
    // 精确的流哈希表，如果出现哈希冲突，会由std::unordered_map处理
    using CounterMap = std::unordered_map<FlowKeyType, uint64_t>;

    // 找到计数最小的流
    typename CounterMap::iterator find_min();

    CounterMap counters;

    // 最多精确记录的流数
    uint64_t capacity;

   public:
    SampleAndHold(uint64_t capacity);

    void update(const FlowKeyType& flow, int increment = 1) override;
    uint64_t query(const FlowKeyType& flow) override;

    inline uint64_t get_capacity() const { return capacity; }
    inline uint64_t get_size() const { return counters.size(); }

    inline void clear() override { counters.clear(); }
};

#endif /* SAMPLEANDHOLD_H */
