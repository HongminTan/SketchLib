#ifndef SKETCHLEARN_H
#define SKETCHLEARN_H

#include <algorithm>
#include <bitset>
#include <map>
#include <queue>
#include <set>
#include <string>

#include "CountMin.h"
#include "HashFunction.h"
#include "Sketch.h"
#include "TwoTuple.h"

/**
 * @brief SketchLearn
 *
 * 基于位级分层和概率推断的流量统计算法
 * 可以主动发现大流并恢复流ID
 */
class SketchLearn : public Sketch {
   private:
    // 位数 = TwoTuple = src_ip 32位 + dst_ip 32位 = 64位
    static constexpr uint64_t num_bits = 64;
    // CountMin 的行数
    uint64_t num_rows;
    // CountMin 的列数
    uint64_t num_cols;
    // CountMin 的 统计层： 0 层是总流统计层，k 层是第 k 位为1的流统计层
    std::vector<std::unique_ptr<CountMin>> layers;
    std::unique_ptr<HashFunction> hash_function;

    // 推断阈值
    double theta;
    // 每层的均值
    std::vector<double> p;
    // 每层的方差
    std::vector<double> variance;

    // 解码结果缓存
    std::map<TwoTuple, uint64_t> decoded_map;
    bool is_decoded;

    // 辅助函数
    std::bitset<num_bits> flow_to_bits(const TwoTuple& flow) const;
    TwoTuple bits_to_flow(const std::bitset<num_bits>& bits) const;
    std::vector<std::bitset<num_bits>> expand_template(
        const std::string& template_str) const;
    void compute_distribution();
    std::vector<TwoTuple> extract_large_flows(uint64_t row_index,
                                              uint64_t col_index);

   public:
    /**
     * @brief 构造函数
     * @param total_memory 总内存大小
     * @param num_rows CountMin 的行数
     * @param theta 推断阈值
     */
    SketchLearn(uint64_t total_memory,
                uint64_t num_rows = 1,
                double theta = 0.5,
                std::unique_ptr<HashFunction> hash_function = nullptr);

    SketchLearn(const SketchLearn& other);
    SketchLearn& operator=(const SketchLearn& other);
    ~SketchLearn() = default;

    void update(const TwoTuple& flow, int increment = 1) override;
    uint64_t query(const TwoTuple& flow) override;

    /**
     * @brief 解码：提取大流
     * @return 解码后的流-计数映射
     */
    std::map<TwoTuple, uint64_t> decode();

    inline void clear() override {
        for (auto& layer : layers) {
            layer->clear();
        }
        decoded_map.clear();
        is_decoded = false;
        std::fill(p.begin(), p.end(), 0.0);
        std::fill(variance.begin(), variance.end(), 0.0);
    }

    static constexpr uint64_t get_num_bits() { return num_bits; }
    inline uint64_t get_num_rows() const { return num_rows; }
    inline uint64_t get_num_cols() const { return num_cols; }
    inline double get_theta() const { return theta; }
};

#endif /* SKETCHLEARN_H */
