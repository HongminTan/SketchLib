#include "HashPipe.h"

HashPipe::HashPipe(uint64_t total_memory,
                   uint64_t num_stages,
                   std::unique_ptr<HashFunction> hash_function)
    : num_stages(num_stages), hash_function(std::move(hash_function)) {
    if (!this->hash_function) {
        this->hash_function = std::make_unique<DefaultHashFunction>();
    }

    // 计算每个 stage 的桶数
    buckets_per_stage = total_memory / num_stages / HPBUCKET_SIZE;

    // 初始化多级哈希表
    stages = std::vector<std::vector<HPBucket>>(
        num_stages, std::vector<HPBucket>(buckets_per_stage));
}

HashPipe::HashPipe(const HashPipe& other)
    : num_stages(other.num_stages),
      buckets_per_stage(other.buckets_per_stage),
      stages(other.stages),
      hash_function(other.hash_function
                        ? other.hash_function->clone()
                        : std::make_unique<DefaultHashFunction>()) {}

HashPipe& HashPipe::operator=(const HashPipe& other) {
    if (this != &other) {
        num_stages = other.num_stages;
        buckets_per_stage = other.buckets_per_stage;
        stages = other.stages;
        hash_function = other.hash_function
                            ? other.hash_function->clone()
                            : std::make_unique<DefaultHashFunction>();
    }
    return *this;
}

void HashPipe::update(const TwoTuple& flow, int increment) {
    // HashPipe 设计为逐包处理
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
            TwoTuple evicted_flow = stages[0][index].flow_id;
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
                    // 继续到下一级
                } else {
                    // 当前流计数较小，被过滤掉
                    break;
                }
            }
        }
    }
}

uint64_t HashPipe::query(const TwoTuple& flow) {
    // 在所有 stage 中查找该流
    for (uint64_t stage = 0; stage < num_stages; stage++) {
        uint64_t index = hash_function->hash(flow, stage, buckets_per_stage);

        if (stages[stage][index].flow_id == flow) {
            return stages[stage][index].count;
        }
    }

    // 找不到返回 0
    return 0;
}
