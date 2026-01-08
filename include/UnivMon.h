#ifndef UNIVMON_H
#define UNIVMON_H

#include <cstdint>
#include <iostream>
#include <memory>
#include <random>

#include "CountSketch.h"
#include "FlowKey.h"
#include "HashFunction.h"
#include "SampleAndHold.h"
#include "Sketch.h"

enum class UnivMonBackend { SaH, CountSketch };

/**
 * @brief 具有分层采样的通用框架 sketch
 *
 * 使用分层采样实现多分辨率流监控。
 * 每一层以递减的概率（1.0、0.5、0.25、...）采样流。
 * 支持两种后端：Sample-and-Hold（精确）或 Count-Sketch（近似）。
 */
template <typename FlowKeyType, typename SFINAE = RequireFlowKey<FlowKeyType>>
class UnivMon : public Sketch<FlowKeyType> {
   private:
    uint64_t num_layers;
    uint64_t total_memory_bytes;
    UnivMonBackend backend;
    std::unique_ptr<HashFunction<FlowKeyType>> hash_function;
    std::vector<std::unique_ptr<Sketch<FlowKeyType>>> layers;
    mutable std::mt19937 rng;

    void initialize_layers();
    // 计算第 layer 层的采样率
    double sample_probability(uint64_t layer) const;
    // 将第 layer 层的观测值放大到原始频率
    uint64_t scale_observation(uint64_t observed, uint64_t layer) const;

   public:
    UnivMon(uint64_t num_layers,
            uint64_t total_memory_bytes,
            std::unique_ptr<HashFunction<FlowKeyType>> hash_function = nullptr,
            UnivMonBackend backend = UnivMonBackend::CountSketch);

    UnivMon(const UnivMon& other);
    UnivMon& operator=(const UnivMon& other);
    ~UnivMon() = default;

    void update(const FlowKeyType& flow, int increment = 1) override;
    uint64_t query(const FlowKeyType& flow) const override;

    inline uint64_t get_layer_count() const { return num_layers; }
    inline uint64_t get_memory_budget() const { return total_memory_bytes; }
    inline UnivMonBackend get_backend() const { return backend; }

    inline void clear() override {
        for (auto& layer : layers) {
            layer->clear();
        }
    }
};

#endif /* UNIVMON_H */