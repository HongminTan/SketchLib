#include "HashPipe.h"

template <typename FlowKeyType, typename SFINAE>
HashPipe<FlowKeyType, SFINAE>::HashPipe(
    uint64_t total_memory,
    uint64_t num_stages,
    std::unique_ptr<HashFunction<FlowKeyType>> hash_function)
    : num_stages(num_stages), hash_function(std::move(hash_function)) {
    if (!this->hash_function) {
        this->hash_function =
            std::make_unique<DefaultHashFunction<FlowKeyType>>();
    }

    // 计算每个 stage 的桶数
    uint64_t bucket_size = sizeof(HPBucket<FlowKeyType>);
    buckets_per_stage = total_memory / num_stages / bucket_size;

    // 初始化多级哈希表
    stages = std::vector<std::vector<HPBucket<FlowKeyType>>>(
        num_stages, std::vector<HPBucket<FlowKeyType>>(buckets_per_stage));
}

template <typename FlowKeyType, typename SFINAE>
HashPipe<FlowKeyType, SFINAE>::HashPipe(const HashPipe& other)
    : num_stages(other.num_stages),
      buckets_per_stage(other.buckets_per_stage),
      stages(other.stages),
      hash_function(
          other.hash_function
              ? other.hash_function->clone()
              : std::make_unique<DefaultHashFunction<FlowKeyType>>()) {}

template <typename FlowKeyType, typename SFINAE>
HashPipe<FlowKeyType, SFINAE>& HashPipe<FlowKeyType, SFINAE>::operator=(
    const HashPipe& other) {
    if (this != &other) {
        num_stages = other.num_stages;
        buckets_per_stage = other.buckets_per_stage;
        stages = other.stages;
        hash_function =
            other.hash_function
                ? other.hash_function->clone()
                : std::make_unique<DefaultHashFunction<FlowKeyType>>();
    }
    return *this;
}

template <typename FlowKeyType, typename SFINAE>
void HashPipe<FlowKeyType, SFINAE>::update(const FlowKeyType& flow,
                                           int increment) {
    // HashPipe 应为逐包处理
    for (int inc = 0; inc < increment; inc++) {
        // stage 0 处理
        uint64_t index = hash_function->hash(flow, 0, buckets_per_stage);

        if (stages[0][index].flow_id == flow) {
            // 匹配，计数+1
            stages[0][index].count++;
        } else if (stages[0][index].empty()) {
            // 空桶，直接插入
            stages[0][index].flow_id = flow;
            stages[0][index].count = 1;
        } else {
            // 冲突：将旧流踢到后续 stage ，新流占据 stage 0
            FlowKeyType evicted_flow = stages[0][index].flow_id;
            uint32_t evicted_count = stages[0][index].count;

            stages[0][index].flow_id = flow;
            stages[0][index].count = 1;

            // 将被踢出的流推进到后续 stage
            for (uint64_t stage = 1; stage < num_stages; stage++) {
                uint64_t idx =
                    hash_function->hash(evicted_flow, stage, buckets_per_stage);

                if (stages[stage][idx].flow_id == evicted_flow) {
                    // 找到了，累加计数
                    stages[stage][idx].count += evicted_count;
                    break;
                }

                if (stages[stage][idx].empty()) {
                    // 空桶，插入
                    stages[stage][idx].flow_id = evicted_flow;
                    stages[stage][idx].count = evicted_count;
                    break;
                }

                if (stages[stage][idx].count < evicted_count) {
                    // 当前流计数更大，替换并继续推进 stage
                    std::swap(evicted_flow, stages[stage][idx].flow_id);
                    std::swap(evicted_count, stages[stage][idx].count);
                } else {
                    // 当前流计数较小，不替换，进入下一级
                    continue;
                }
            }
        }
    }
}

template <typename FlowKeyType, typename SFINAE>
uint64_t HashPipe<FlowKeyType, SFINAE>::query(const FlowKeyType& flow) {
    // 在所有 stage 中查找该流
    uint64_t count = 0;
    for (uint64_t stage = 0; stage < num_stages; stage++) {
        uint64_t index = hash_function->hash(flow, stage, buckets_per_stage);

        if (stages[stage][index].flow_id == flow) {
            count += stages[stage][index].count;
        }
    }

    // 找不到返回 0
    return count;
}

template class HashPipe<OneTuple>;
template class HashPipe<TwoTuple>;
template class HashPipe<FiveTuple>;
