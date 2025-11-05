#ifndef ELASTICSKETCH_H
#define ELASTICSKETCH_H

#include "CountMin.h"
#include "HashFunction.h"
#include "Sketch.h"
#include "TwoTuple.h"

// HeavyBucket 的固定大小
// TwoTuple(8字节) + posvote(4字节) + negvote(4字节) + flag(1字节) + padding
constexpr uint64_t HEAVY_BUCKET_SIZE = 20;

// Heavy Part 的哈希种子是第 104 个质数
constexpr uint64_t HEAVY_PART_SEED = 104;

/**
 * @brief Heavy Part 的桶结构
 *
 * 存储流标识符和投票计数器，用于识别和存储大流
 */
struct HeavyBucket {
    // 流标识符
    TwoTuple flow_id;
    // 正票
    uint32_t pos_vote;
    // 负票
    uint32_t neg_vote;
    // 该桶是否发生过替换
    bool flag;

    HeavyBucket() : flow_id(), pos_vote(0), neg_vote(0), flag(false) {}

    inline bool empty() const {
        return flow_id.src_ip == 0 && flow_id.dst_ip == 0;
    }

    inline void clear() {
        flow_id = TwoTuple();
        pos_vote = 0;
        neg_vote = 0;
        flag = false;
    }
};

/**
 * @brief Heavy Part - 精确存储大流
 *
 * 使用投票机制动态识别和替换流，保证大流的精确计数
 */
class HeavyPart {
   private:
    // 替换阈值
    uint64_t lambda;
    // 桶数量
    uint64_t bucket_count;
    std::vector<HeavyBucket> buckets;
    std::unique_ptr<HashFunction> hash_function;

   public:
    HeavyPart(uint64_t bucket_count,
              uint64_t lambda,
              std::unique_ptr<HashFunction> hash_function = nullptr);

    HeavyPart(const HeavyPart& other);
    HeavyPart& operator=(const HeavyPart& other);

    /**
     * @brief 更新流
     * @param flow 流标识符
     * @return 0 表示成功插入/更新，>0 表示被踢出的旧流的计数值
     */
    uint64_t update(TwoTuple& flow);

    /**
     * @brief 查询流的计数
     * @param flow 流标识符
     * @param flag 输出参数，表示该位置是否发生过替换
     * @return 流的计数值
     */
    uint64_t query(const TwoTuple& flow, bool& flag) const;

    inline void clear() {
        for (auto& bucket : buckets) {
            bucket.clear();
        }
    }

    inline const std::vector<HeavyBucket>& get_buckets() const {
        return buckets;
    }
};

/**
 * @brief Light Part - 近似存储小流
 *
 * ElasticSketch 的 Light Part 使用 CountMin Sketch 实现
 */
using LightPart = CountMin;

/**
 * @brief ElasticSketch
 *
 * 使用 Heavy Part 精确记录大流，Light Part 近似记录小流
 * 通过投票机制实现自适应的流识别和替换
 */
class ElasticSketch : public Sketch {
   private:
    std::unique_ptr<HeavyPart> heavy_part;
    std::unique_ptr<LightPart> light_part;
    // Heavy Part 的内存大小
    uint64_t heavy_memory;
    // 投票替换阈值: neg_vote/pos_vote >= lambda 时触发替换
    uint64_t lambda;

   public:
    /**
     * @brief 构造函数
     * @param heavy_memory Heavy Part 的内存大小
     * @param lambda 投票替换阈值
     * @param total_memory 总内存大小
     * @param light_rows
     * @param hash_function
     */
    ElasticSketch(uint64_t heavy_memory,
                  uint64_t lambda,
                  uint64_t total_memory,
                  uint64_t light_rows = 8,
                  std::unique_ptr<HashFunction> hash_function = nullptr);

    ElasticSketch(const ElasticSketch& other);
    ElasticSketch& operator=(const ElasticSketch& other);
    ~ElasticSketch() = default;

    void update(const TwoTuple& flow, int increment = 1) override;

    uint64_t query(const TwoTuple& flow) override;

    inline void clear() override {
        heavy_part->clear();
        light_part->clear();
    }

    /**
     * @brief 获取 Heavy Part 的桶数量
     */
    inline uint64_t get_heavy_bucket_count() const {
        return heavy_memory / HEAVY_BUCKET_SIZE;
    }

    /**
     * @brief 获取 Light Part 的行数和列数
     */
    inline std::pair<uint64_t, uint64_t> get_light_size() const {
        return {light_part->get_rows(), light_part->get_cols()};
    }

    inline uint64_t get_lambda() const { return lambda; }
};

#endif /* ELASTICSKETCH_H */
