#ifndef HASHPIPE_H
#define HASHPIPE_H

#include <vector>

#include "HashFunction.h"
#include "Sketch.h"
#include "TwoTuple.h"

// HashPipe 桶大小：TwoTuple(8字节) + count(4字节)
constexpr uint64_t HPBUCKET_SIZE = 12;

/**
 * @brief HashPipe 的桶结构
 */
struct HPBucket {
    // 流标识符
    TwoTuple flow_id;
    // 计数
    uint32_t count;

    HPBucket() : flow_id(), count(0) {}

    inline bool empty() const { return count == 0; }

    inline void clear() {
        flow_id = TwoTuple();
        count = 0;
    }
};

/**
 * @brief HashPipe
 *
 * 使用多级流水线结构，大流会沉淀在某一级，小流逐级推进并被过滤
 */
class HashPipe : public Sketch {
   private:
    // stage 数
    uint64_t num_stages;
    // 每个 stage 的桶数
    uint64_t buckets_per_stage;
    // 多级哈希表 [stage][bucket]
    std::vector<std::vector<HPBucket>> stages;
    std::unique_ptr<HashFunction> hash_function;

   public:
    /**
     * @brief 构造函数
     * @param total_memory 总内存大小
     * @param num_stages 阶段数
     * @param hash_function
     */
    HashPipe(uint64_t total_memory,
             uint64_t num_stages = 8,
             std::unique_ptr<HashFunction> hash_function = nullptr);

    HashPipe(const HashPipe& other);
    HashPipe& operator=(const HashPipe& other);
    ~HashPipe() = default;

    void update(const TwoTuple& flow, int increment = 1) override;
    uint64_t query(const TwoTuple& flow) override;

    inline void clear() override {
        for (auto& stage : stages) {
            for (auto& bucket : stage) {
                bucket.clear();
            }
        }
    }

    inline uint64_t get_num_stages() const { return num_stages; }
    inline uint64_t get_buckets_per_stage() const { return buckets_per_stage; }
};

#endif /* HASHPIPE_H */
