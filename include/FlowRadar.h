#ifndef FLOWRADAR_H
#define FLOWRADAR_H

#include <map>

#include "BloomFilter.h"
#include "FlowKey.h"
#include "HashFunction.h"
#include "Sketch.h"

/**
 * @brief FlowRadar 的桶结构
 */
template <typename FlowKeyType>
struct FRBucket {
    // 流ID的异或和
    FlowKeyType flow_xor;
    // 流数量
    uint32_t flow_count;
    // 包数量
    uint32_t packet_count;

    FRBucket() : flow_xor(), flow_count(0), packet_count(0) {}

    inline void clear() {
        flow_xor = FlowKeyType();
        flow_count = 0;
        packet_count = 0;
    }
};

/**
 * @brief FlowRadar
 *
 * 基于 XOR 编码和迭代解码的流量统计算法
 * 理论上可以恢复所有流及其精确频率
 */
template <typename FlowKeyType, typename SFINAE = RequireFlowKey<FlowKeyType>>
class FlowRadar : public Sketch<FlowKeyType> {
   private:
    // BloomFilter 哈希函数数量
    uint64_t bf_num_hashes;
    // CountingTable 哈希函数数量
    uint64_t ct_num_hashes;
    // BloomFilter
    std::unique_ptr<BloomFilter<FlowKeyType>> bloom_filter;
    // CountingTable
    std::vector<FRBucket<FlowKeyType>> counting_table;
    std::unique_ptr<HashFunction<FlowKeyType>> hash_function;
    // 解码结果缓存
    std::map<FlowKeyType, uint64_t> decoded_map;
    bool is_decoded;

   public:
    /**
     * @brief 构造函数
     * @param total_memory 总内存大小
     * @param bf_percentage BloomFilter 占用内存比例（0-1）
     * @param bf_num_hashes BloomFilter 哈希函数数量
     * @param ct_num_hashes CountingTable 哈希函数数量
     */
    FlowRadar(
        uint64_t total_memory,
        double bf_percentage = 0.3,
        uint64_t bf_num_hashes = 3,
        uint64_t ct_num_hashes = 3,
        std::unique_ptr<HashFunction<FlowKeyType>> hash_function = nullptr);

    FlowRadar(const FlowRadar& other);
    FlowRadar& operator=(const FlowRadar& other);
    ~FlowRadar() = default;

    void update(const FlowKeyType& flow, int increment = 1) override;
    uint64_t query(const FlowKeyType& flow) override;

    /**
     * @brief 解码流信息
     * @return 解码后的流-计数映射
     */
    std::map<FlowKeyType, uint64_t> decode();

    inline void clear() override {
        bloom_filter->clear();
        for (auto& bucket : counting_table) {
            bucket.clear();
        }
        decoded_map.clear();
        is_decoded = false;
    }

    inline uint64_t get_bf_num_hashes() const { return bf_num_hashes; }
    inline uint64_t get_ct_num_hashes() const { return ct_num_hashes; }
    inline uint64_t get_table_size() const { return counting_table.size(); }
};

#endif /* FLOWRADAR_H */
