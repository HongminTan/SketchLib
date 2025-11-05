#include "ElasticSketch.h"

HeavyPart::HeavyPart(uint64_t bucket_count,
                     uint64_t lambda,
                     std::unique_ptr<HashFunction> hash_function)
    : lambda(lambda),
      bucket_count(bucket_count),
      buckets(bucket_count),
      hash_function(std::move(hash_function)) {
    if (!this->hash_function) {
        this->hash_function = std::make_unique<DefaultHashFunction>();
    }
}

HeavyPart::HeavyPart(const HeavyPart& other)
    : lambda(other.lambda),
      bucket_count(other.bucket_count),
      buckets(other.buckets),
      hash_function(other.hash_function
                        ? other.hash_function->clone()
                        : std::make_unique<DefaultHashFunction>()) {}

HeavyPart& HeavyPart::operator=(const HeavyPart& other) {
    if (this != &other) {
        lambda = other.lambda;
        bucket_count = other.bucket_count;
        buckets = other.buckets;
        hash_function = other.hash_function
                            ? other.hash_function->clone()
                            : std::make_unique<DefaultHashFunction>();
    }
    return *this;
}

uint64_t HeavyPart::update(TwoTuple& flow) {
    // seed 是 第 104 个质数
    uint64_t index = hash_function->hash(flow, 104, bucket_count);

    // 桶为空，直接插入
    if (buckets[index].empty()) {
        buckets[index].flow_id = flow;
        buckets[index].pos_vote = 1;
        return 0;
    }

    // 是同一个流，增加正票
    if (buckets[index].flow_id == flow) {
        buckets[index].pos_vote++;
        return 0;
    }

    // 是不同的流，增加负票
    buckets[index].neg_vote++;

    // 检查是否需要替换
    uint64_t vote_ratio = buckets[index].neg_vote / buckets[index].pos_vote;

    if (vote_ratio < lambda) {
        // 不替换，返回1表示需要更新到 Light Part
        return 1;
    } else {
        // 旧流被踢出
        uint64_t evicted_count = buckets[index].pos_vote;

        // 标记发生过替换
        buckets[index].flag = true;

        // 将旧流换入 flow 中
        std::swap(buckets[index].flow_id, flow);

        // 重置投票计数
        buckets[index].pos_vote = 1;
        buckets[index].neg_vote = 0;

        // 返回被踢出的旧流的计数
        return evicted_count;
    }
}

uint64_t HeavyPart::query(const TwoTuple& flow, bool& flag) const {
    uint64_t pos = hash_function->hash(flow, 104, bucket_count);

    flag = buckets[pos].flag;

    if (buckets[pos].flow_id == flow) {
        return buckets[pos].pos_vote;
    }

    return 0;
}

ElasticSketch::ElasticSketch(uint64_t heavy_memory,
                             uint64_t lambda,
                             uint64_t total_memory,
                             uint64_t light_rows,
                             std::unique_ptr<HashFunction> hash_function)
    : heavy_memory(heavy_memory), lambda(lambda) {
    // 计算 Heavy Part 的桶数量
    uint64_t heavy_bucket_count = heavy_memory / HEAVY_BUCKET_SIZE;

    std::unique_ptr<HashFunction> heavy_hash;
    std::unique_ptr<HashFunction> light_hash;
    if (hash_function) {
        heavy_hash = hash_function->clone();
        light_hash = hash_function->clone();
    } else {
        heavy_hash = std::make_unique<DefaultHashFunction>();
        light_hash = std::make_unique<DefaultHashFunction>();
    }

    heavy_part = std::make_unique<HeavyPart>(heavy_bucket_count, lambda,
                                             std::move(heavy_hash));

    // 计算 Light Part 的内存
    uint64_t light_memory = total_memory - heavy_memory;
    light_part = std::make_unique<LightPart>(light_rows, light_memory,
                                             std::move(light_hash));
}

ElasticSketch::ElasticSketch(const ElasticSketch& other)
    : heavy_memory(other.heavy_memory), lambda(other.lambda) {
    heavy_part = std::make_unique<HeavyPart>(*other.heavy_part);
    light_part = std::make_unique<LightPart>(*other.light_part);
}

ElasticSketch& ElasticSketch::operator=(const ElasticSketch& other) {
    if (this != &other) {
        heavy_memory = other.heavy_memory;
        lambda = other.lambda;
        heavy_part = std::make_unique<HeavyPart>(*other.heavy_part);
        light_part = std::make_unique<LightPart>(*other.light_part);
    }
    return *this;
}

void ElasticSketch::update(const TwoTuple& flow, int increment) {
    // ElasticSketch 应逐包处理
    for (int i = 0; i < increment; i++) {
        // heavy_part 会将被踢出的流放到 flow_ 中
        TwoTuple flow_ = flow;
        uint64_t evicted_count = heavy_part->update(flow_);

        // flow_ 是需要被存到 Light Part 的流
        if (evicted_count > 0) {
            int evicted_int = (evicted_count > INT32_MAX)
                                  ? INT32_MAX
                                  : static_cast<int>(evicted_count);
            light_part->update(flow_, evicted_int);
        }
    }
}

uint64_t ElasticSketch::query(const TwoTuple& flow) {
    bool flag = false;
    uint64_t heavy_count = heavy_part->query(flow, flag);

    // 如果该桶发生过替换，需要加上 Light Part 的计数
    if (flag) {
        uint64_t light_count = light_part->query(flow);
        return heavy_count + light_count;
    }

    return heavy_count;
}
