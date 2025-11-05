/**
 * @file example.cpp
 * @brief SketchLib 完整使用示例
 *
 * 包含所有 Sketch 算法的使用示例：
 * - Count-Min Sketch
 * - Count Sketch
 * - Sample-and-Hold
 * - UnivMon
 * - 自定义哈希函数
 */

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <vector>

// SketchLib 头文件
#include "CountMin.h"
#include "CountSketch.h"
#include "ElasticSketch.h"
#include "HashFunction.h"
#include "SampleAndHold.h"
#include "TwoTuple.h"
#include "UnivMon.h"

// ==================== 辅助函数 ====================

// 将 IP 字符串转换为 uint32_t
uint32_t ip_to_uint32(const char* ip) {
    unsigned char bytes[4];
    if (sscanf(ip, "%hhu.%hhu.%hhu.%hhu", &bytes[0], &bytes[1], &bytes[2],
               &bytes[3]) != 4) {
        return 0;
    }
    return (static_cast<uint32_t>(bytes[0]) << 24) |
           (static_cast<uint32_t>(bytes[1]) << 16) |
           (static_cast<uint32_t>(bytes[2]) << 8) |
           static_cast<uint32_t>(bytes[3]);
}

// 将 uint32_t 转换为 IP 字符串
std::string uint32_to_ip(uint32_t ip) {
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%u.%u.%u.%u", (ip >> 24) & 0xFF,
             (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, ip & 0xFF);
    return std::string(buffer);
}

void print_separator(const std::string& title) {
    std::cout << "\n" << std::string(70, '=') << std::endl;
    std::cout << title << std::endl;
    std::cout << std::string(70, '=') << std::endl;
}

// ==================== Count-Min Sketch 示例 ====================

void example_countmin_basic() {
    print_separator("示例 1: Count-Min Sketch 基本使用");

    // 创建 Count-Min Sketch：4 行，1024 字节内存
    CountMin cm(4, 1024);

    std::cout << "\n创建 Count-Min Sketch: " << cm.get_rows() << " 行 × "
              << cm.get_cols() << " 列" << std::endl;

    // 创建流标识符
    TwoTuple flow1(ip_to_uint32("192.168.1.1"), ip_to_uint32("10.0.0.1"));
    TwoTuple flow2(ip_to_uint32("192.168.1.2"), ip_to_uint32("10.0.0.2"));

    // 更新流计数
    cm.update(flow1, 100);
    cm.update(flow2, 50);
    cm.update(flow1, 20);

    // 查询流计数
    std::cout << "流 1 计数: " << cm.query(flow1) << " (实际: 120)"
              << std::endl;
    std::cout << "流 2 计数: " << cm.query(flow2) << " (实际: 50)" << std::endl;

    std::cout << "\n特点: Count-Min Sketch 永远不会低估，但可能高估"
              << std::endl;
}

void example_countmin_heavy_hitter() {
    print_separator("示例 2: Count-Min 重流检测");

    CountMin cm(5, 2048);

    // 模拟流量数据
    struct FlowData {
        TwoTuple flow;
        std::string name;
        int packets;
    };

    std::vector<FlowData> flows = {
        {TwoTuple(ip_to_uint32("192.168.1.1"), ip_to_uint32("10.0.0.1")),
         "Web Server", 1000},
        {TwoTuple(ip_to_uint32("192.168.1.2"), ip_to_uint32("10.0.0.2")),
         "Database", 850},
        {TwoTuple(ip_to_uint32("192.168.1.3"), ip_to_uint32("10.0.0.3")),
         "API Server", 50},
        {TwoTuple(ip_to_uint32("192.168.1.4"), ip_to_uint32("10.0.0.4")),
         "Cache", 30},
    };

    int total_packets = 0;
    for (const auto& flow_data : flows) {
        cm.update(flow_data.flow, flow_data.packets);
        total_packets += flow_data.packets;
    }

    std::cout << "\n总数据包: " << total_packets << std::endl;
    std::cout << "检测重流（阈值：10% 总流量）...\n" << std::endl;

    // 检测重流
    double threshold = 0.1 * total_packets;
    for (const auto& flow_data : flows) {
        uint64_t count = cm.query(flow_data.flow);
        if (count >= threshold) {
            double percentage = 100.0 * count / total_packets;
            std::cout << "  " << flow_data.name << ": " << count << " 包 ("
                      << std::fixed << std::setprecision(1) << percentage
                      << "%)" << std::endl;
        }
    }
}

// ==================== Count Sketch 示例 ====================

void example_countsketch_basic() {
    print_separator("示例 3: Count Sketch 基本使用");

    // 创建 Count Sketch：5 行，1024 字节内存
    CountSketch cs(5, 1024);

    std::cout << "\n创建 Count Sketch: " << cs.get_rows() << " 行 × "
              << cs.get_cols() << " 列" << std::endl;

    TwoTuple flow1(ip_to_uint32("192.168.1.1"), ip_to_uint32("10.0.0.1"));
    TwoTuple flow2(ip_to_uint32("192.168.1.2"), ip_to_uint32("10.0.0.2"));

    cs.update(flow1, 100);
    cs.update(flow2, 50);

    std::cout << "流 1 计数: " << cs.query(flow1) << " (实际: 100)"
              << std::endl;
    std::cout << "流 2 计数: " << cs.query(flow2) << " (实际: 50)" << std::endl;

    std::cout << "\n特点: Count Sketch 使用中位数估计，提供无偏估计"
              << std::endl;
}

void example_countsketch_vs_countmin() {
    print_separator("示例 4: Count Sketch vs Count-Min 对比");

    CountSketch cs(5, 2048);
    CountMin cm(5, 2048);

    std::cout << "\n相同配置: 5 行，2048 字节内存" << std::endl;

    // 1 个重流 + 50 个轻流
    TwoTuple heavy_flow(ip_to_uint32("192.168.1.1"), ip_to_uint32("10.0.0.1"));
    int heavy_count = 10000;

    cs.update(heavy_flow, heavy_count);
    cm.update(heavy_flow, heavy_count);

    for (int i = 0; i < 50; i++) {
        TwoTuple flow(ip_to_uint32("192.168.2.1") + i,
                      ip_to_uint32("10.0.0.100"));
        cs.update(flow, 10);
        cm.update(flow, 10);
    }

    uint64_t cs_heavy = cs.query(heavy_flow);
    uint64_t cm_heavy = cm.query(heavy_flow);

    std::cout << "\n重流查询结果:" << std::endl;
    std::cout << "  实际计数:      " << heavy_count << std::endl;
    std::cout << "  Count Sketch:  " << cs_heavy
              << " (误差: " << std::abs((int64_t)cs_heavy - heavy_count) << ")"
              << std::endl;
    std::cout << "  Count-Min:     " << cm_heavy
              << " (误差: " << std::abs((int64_t)cm_heavy - heavy_count) << ")"
              << std::endl;

    std::cout << "\n结论: Count-Min 可能高估，Count Sketch 期望无偏"
              << std::endl;
}

// ==================== Sample-and-Hold 示例 ====================

void example_sampleandhold_basic() {
    print_separator("示例 5: Sample-and-Hold 基本使用");

    // 创建 Sample-and-Hold：容量 10 个流
    SampleAndHold sah(10);

    std::cout << "\n创建 Sample-and-Hold: 容量 = " << sah.get_capacity()
              << std::endl;

    TwoTuple flow1(ip_to_uint32("192.168.1.1"), ip_to_uint32("10.0.0.1"));
    TwoTuple flow2(ip_to_uint32("192.168.1.2"), ip_to_uint32("10.0.0.2"));

    sah.update(flow1, 100);
    sah.update(flow2, 50);
    sah.update(flow1, 20);

    std::cout << "流 1 计数: " << sah.query(flow1) << " (精确值: 120)"
              << std::endl;
    std::cout << "流 2 计数: " << sah.query(flow2) << " (精确值: 50)"
              << std::endl;
    std::cout << "当前大小: " << sah.get_size() << "/" << sah.get_capacity()
              << std::endl;

    std::cout << "\n特点: Sample-and-Hold 提供 100% 精确的计数" << std::endl;
}

void example_sampleandhold_eviction() {
    print_separator("示例 6: Sample-and-Hold 驱逐策略");

    SampleAndHold sah(3);  // 容量为 3

    std::cout << "\n创建容量为 3 的 Sample-and-Hold" << std::endl;

    TwoTuple flow1(ip_to_uint32("192.168.1.1"), ip_to_uint32("10.0.0.1"));
    TwoTuple flow2(ip_to_uint32("192.168.1.2"), ip_to_uint32("10.0.0.2"));
    TwoTuple flow3(ip_to_uint32("192.168.1.3"), ip_to_uint32("10.0.0.3"));
    TwoTuple flow4(ip_to_uint32("192.168.1.4"), ip_to_uint32("10.0.0.4"));

    // 填满容量
    sah.update(flow1, 100);
    sah.update(flow2, 50);
    sah.update(flow3, 20);  // 最小

    std::cout << "\n步骤 1: 添加 3 个流（容量已满）" << std::endl;
    std::cout << "  流 1: " << sah.query(flow1) << " 包" << std::endl;
    std::cout << "  流 2: " << sah.query(flow2) << " 包" << std::endl;
    std::cout << "  流 3: " << sah.query(flow3) << " 包 (最小)" << std::endl;

    // 添加更大的流，驱逐最小流
    sah.update(flow4, 80);

    std::cout << "\n步骤 2: 添加流 4 (80 包，大于最小值 20)" << std::endl;
    std::cout << "  流 3 被驱逐: " << (sah.has_flow(flow3) ? "否" : "是")
              << std::endl;
    std::cout << "  流 4 被接纳: " << (sah.has_flow(flow4) ? "是" : "否")
              << std::endl;
    std::cout << "  当前大小: " << sah.get_size() << "/" << sah.get_capacity()
              << std::endl;
}

void example_sampleandhold_heavy_hitter() {
    print_separator("示例 7: Sample-and-Hold 重流检测");

    const int num_heavy = 5;
    const int num_light = 100;

    SampleAndHold sah(num_heavy * 2);  // 容量为预期重流数的 2 倍

    std::cout << "\n模拟流量: " << num_heavy << " 个重流 (各 1000 包) + "
              << num_light << " 个轻流 (各 10 包)" << std::endl;
    std::cout << "Sample-and-Hold 容量: " << sah.get_capacity() << std::endl;

    // 添加重流
    std::vector<TwoTuple> heavy_flows;
    for (int i = 0; i < num_heavy; i++) {
        heavy_flows.emplace_back(ip_to_uint32("192.168.1.1") + i,
                                 ip_to_uint32("10.0.0.1"));
        sah.update(heavy_flows.back(), 1000);
    }

    // 添加轻流
    int captured_light = 0;
    for (int i = 0; i < num_light; i++) {
        TwoTuple flow(ip_to_uint32("192.168.2.1") + i,
                      ip_to_uint32("10.0.0.2"));
        sah.update(flow, 10);
        if (sah.has_flow(flow)) {
            captured_light++;
        }
    }

    // 检查捕获情况
    int captured_heavy = 0;
    for (const auto& flow : heavy_flows) {
        if (sah.has_flow(flow)) {
            captured_heavy++;
        }
    }

    std::cout << "\n结果:" << std::endl;
    std::cout << "  追踪的流: " << sah.get_size() << std::endl;
    std::cout << "  捕获重流: " << captured_heavy << "/" << num_heavy
              << " (100%)" << std::endl;
    std::cout << "  捕获轻流: " << captured_light << "/" << num_light << " ("
              << (100.0 * captured_light / num_light) << "%)" << std::endl;

    std::cout << "\n结论: 精确捕获所有重流，过滤大部分轻流" << std::endl;
}

// ==================== UnivMon 示例 ====================

void example_univmon_basic() {
    print_separator("示例 8: UnivMon 基本使用");

    // 创建 UnivMon：4 层，4096 字节，CountSketch 后端
    UnivMon um(4, 4096, nullptr, UnivMonBackend::CountSketch);

    std::cout << "\n创建 UnivMon:" << std::endl;
    std::cout << "  层数: " << um.get_layer_count() << std::endl;
    std::cout << "  总内存: " << um.get_memory_budget() << " 字节" << std::endl;
    std::cout << "  后端: "
              << (um.get_backend() == UnivMonBackend::CountSketch
                      ? "Count Sketch"
                      : "Sample-and-Hold")
              << std::endl;

    std::cout << "\n分层采样概率:" << std::endl;
    for (uint64_t layer = 0; layer < 4; layer++) {
        double prob = 1.0 / std::pow(2, layer);
        std::cout << "  第 " << layer << " 层: " << std::fixed
                  << std::setprecision(4) << prob << std::endl;
    }

    TwoTuple flow(ip_to_uint32("192.168.1.1"), ip_to_uint32("10.0.0.1"));
    um.update(flow, 100);

    std::cout << "\n流计数: " << um.query(flow) << " (更新值: 100)"
              << std::endl;
    std::cout << "\n特点: 多分辨率监控，自适应采样" << std::endl;
}

void example_univmon_multi_scale() {
    print_separator("示例 9: UnivMon 多尺度分析");

    UnivMon um(5, 8192, nullptr, UnivMonBackend::CountSketch);

    // 不同大小的流
    std::vector<std::pair<TwoTuple, int>> flows = {
        {TwoTuple(ip_to_uint32("192.168.1.1"), ip_to_uint32("10.0.0.1")),
         10000},  // 重流
        {TwoTuple(ip_to_uint32("192.168.1.2"), ip_to_uint32("10.0.0.2")),
         1000},  // 中流
        {TwoTuple(ip_to_uint32("192.168.1.3"), ip_to_uint32("10.0.0.3")),
         100},  // 轻流
    };

    std::cout << "\n更新不同大小的流..." << std::endl;
    for (const auto& f : flows) {
        um.update(f.first, f.second);
    }

    std::cout << "\n查询结果:" << std::endl;
    std::cout << std::setw(15) << "流类型" << std::setw(15) << "实际计数"
              << std::setw(15) << "估计计数" << std::endl;
    std::cout << std::string(45, '-') << std::endl;

    std::vector<std::string> labels = {"重流", "中流", "轻流"};
    for (size_t i = 0; i < flows.size(); i++) {
        uint64_t estimate = um.query(flows[i].first);
        std::cout << std::setw(15) << labels[i] << std::setw(15)
                  << flows[i].second << std::setw(15) << estimate << std::endl;
    }
}

void example_univmon_backend_comparison() {
    print_separator("示例 10: UnivMon 后端对比");

    UnivMon um_sah(4, 8192, nullptr, UnivMonBackend::SaH);
    UnivMon um_cs(4, 8192, nullptr, UnivMonBackend::CountSketch);

    std::cout << "\n配置: 4 层，8192 字节" << std::endl;

    TwoTuple flow(ip_to_uint32("192.168.1.1"), ip_to_uint32("10.0.0.1"));

    um_sah.update(flow, 1000);
    um_cs.update(flow, 1000);

    // 添加噪声
    for (int i = 0; i < 50; i++) {
        TwoTuple noise(ip_to_uint32("192.168.2.1") + i,
                       ip_to_uint32("10.0.0.2"));
        um_sah.update(noise, 10);
        um_cs.update(noise, 10);
    }

    std::cout << "\n流查询 (实际: 1000):" << std::endl;
    std::cout << "  SaH 后端:          " << um_sah.query(flow) << " (精确)"
              << std::endl;
    std::cout << "  CountSketch 后端:  " << um_cs.query(flow) << " (近似)"
              << std::endl;

    std::cout << "\n选择建议:" << std::endl;
    std::cout << "  - SaH: 精确但容量有限，适合重流监控" << std::endl;
    std::cout << "  - CountSketch: 近似但容量大，适合通用监控" << std::endl;
}

// ==================== 自定义哈希函数示例 ====================

// 简单的自定义哈希函数（示例）
class CustomHashFunction : public HashFunction {
   public:
    uint64_t hash(const TwoTuple& flow,
                  uint64_t seed,
                  uint64_t mod) const override {
        const uint64_t PRIME = 0x9e3779b97f4a7c15ULL;
        uint64_t hash_val = seed;
        hash_val ^= flow.src_ip * PRIME;
        hash_val ^= flow.dst_ip * PRIME;
        hash_val *= PRIME;
        hash_val ^= (hash_val >> 33);
        return hash_val % mod;
    }

    std::unique_ptr<HashFunction> clone() const override {
        return std::make_unique<CustomHashFunction>();
    }
};

void example_custom_hash() {
    print_separator("示例 11: 使用自定义哈希函数");

    // 使用自定义哈希
    auto custom_hash = std::make_unique<CustomHashFunction>();
    CountMin cm(5, 2048, std::move(custom_hash));

    std::cout << "\n创建 Count-Min Sketch (使用自定义哈希函数)" << std::endl;

    TwoTuple flow(ip_to_uint32("192.168.1.1"), ip_to_uint32("10.0.0.1"));
    cm.update(flow, 100);

    std::cout << "流计数: " << cm.query(flow) << " (实际: 100)" << std::endl;
    std::cout << "\n特点: 依赖注入设计，可灵活替换哈希函数" << std::endl;
}

void example_hash_comparison() {
    print_separator("示例 12: 哈希函数性能对比");

    const int num_updates = 10000;
    std::vector<TwoTuple> test_flows;

    for (int i = 0; i < 100; i++) {
        test_flows.emplace_back(ip_to_uint32("192.168.1.1") + i,
                                ip_to_uint32("10.0.0.1"));
    }

    std::cout << "\n测试配置: " << num_updates << " 次更新\n" << std::endl;
    std::cout << std::setw(25) << "哈希函数" << std::setw(20) << "时间 (ms)"
              << std::endl;
    std::cout << std::string(45, '-') << std::endl;

    // 测试 MurmurHash3
    {
        auto hash = std::make_unique<MurmurV3HashFunction>();
        CountMin cm(5, 4096, std::move(hash));

        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < num_updates; i++) {
            cm.update(test_flows[i % test_flows.size()], 1);
        }
        auto end = std::chrono::high_resolution_clock::now();
        auto time =
            std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
                .count();

        std::cout << std::setw(25) << "MurmurHash3 (默认)" << std::setw(20)
                  << time << std::endl;
    }

    // 测试 SpookyV2
    {
        auto hash = std::make_unique<SpookyV2HashFunction>();
        CountMin cm(5, 4096, std::move(hash));

        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < num_updates; i++) {
            cm.update(test_flows[i % test_flows.size()], 1);
        }
        auto end = std::chrono::high_resolution_clock::now();
        auto time =
            std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
                .count();

        std::cout << std::setw(25) << "SpookyV2" << std::setw(20) << time
                  << std::endl;
    }

    // 测试自定义哈希
    {
        auto hash = std::make_unique<CustomHashFunction>();
        CountMin cm(5, 4096, std::move(hash));

        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < num_updates; i++) {
            cm.update(test_flows[i % test_flows.size()], 1);
        }
        auto end = std::chrono::high_resolution_clock::now();
        auto time =
            std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
                .count();

        std::cout << std::setw(25) << "Custom (自定义)" << std::setw(20) << time
                  << std::endl;
    }
}

// ==================== 综合示例 ====================

// ==================== ElasticSketch 示例 ====================

void example_elasticsketch_basic() {
    print_separator("示例 13: ElasticSketch 基本使用");

    // 创建 ElasticSketch：
    // - Heavy Part: 2000 字节
    // - Lambda: 3 (替换阈值)
    // - 总内存: 8192 字节
    // - Light Part: 8 行
    ElasticSketch es(2000, 3, 8192, 8);

    std::cout << "\n创建 ElasticSketch:" << std::endl;
    std::cout << "  Heavy 桶数: " << es.get_heavy_bucket_count() << std::endl;
    auto light_size = es.get_light_size();
    std::cout << "  Light 大小: " << light_size.first << " 行 × "
              << light_size.second << " 列" << std::endl;
    std::cout << "  Lambda 值: " << es.get_lambda() << std::endl;

    TwoTuple flow1(ip_to_uint32("192.168.1.1"), ip_to_uint32("10.0.0.1"));
    TwoTuple flow2(ip_to_uint32("192.168.1.2"), ip_to_uint32("10.0.0.2"));

    es.update(flow1, 100);
    es.update(flow2, 50);

    std::cout << "\n流 1 计数: " << es.query(flow1) << " (实际: 100)"
              << std::endl;
    std::cout << "流 2 计数: " << es.query(flow2) << " (实际: 50)" << std::endl;

    std::cout << "\n特点: Heavy Part 精确记录大流，Light Part 近似记录小流"
              << std::endl;
}

void example_elasticsketch_eviction() {
    print_separator("示例 14: ElasticSketch 投票替换机制");

    // 小的 Heavy Part 来演示替换
    ElasticSketch es(400, 2, 2048, 8);

    std::cout << "\n创建小容量 ElasticSketch (Heavy 桶数: "
              << es.get_heavy_bucket_count() << ")" << std::endl;

    TwoTuple heavy_flow(ip_to_uint32("192.168.1.1"), ip_to_uint32("10.0.0.1"));
    TwoTuple competing_flow(ip_to_uint32("192.168.1.2"),
                            ip_to_uint32("10.0.0.2"));

    std::cout << "\n步骤 1: 插入重流 (1000 包)" << std::endl;
    es.update(heavy_flow, 1000);
    std::cout << "  重流计数: " << es.query(heavy_flow) << std::endl;

    std::cout << "\n步骤 2: 插入竞争流 (尝试触发替换)" << std::endl;
    for (int i = 0; i < 100; i++) {
        es.update(competing_flow, 10);
    }

    uint64_t heavy_count = es.query(heavy_flow);
    uint64_t competing_count = es.query(competing_flow);

    std::cout << "  重流计数: " << heavy_count << std::endl;
    std::cout << "  竞争流计数: " << competing_count << std::endl;

    std::cout << "\n说明: 投票机制确保真正的大流占据 Heavy Part" << std::endl;
}

void example_elasticsketch_heavy_hitter() {
    print_separator("示例 15: ElasticSketch 重流检测");

    ElasticSketch es(4000, 2, 16384, 8);

    // 模拟流量：5个重流 + 50个轻流
    std::vector<std::pair<TwoTuple, int>> flows;

    std::cout << "\n模拟流量数据..." << std::endl;
    std::cout << "  5 个重流 (各 1000 包)" << std::endl;
    for (int i = 0; i < 5; i++) {
        TwoTuple flow(ip_to_uint32("192.168.1.1") + i,
                      ip_to_uint32("10.0.0.1"));
        flows.push_back({flow, 1000});
        es.update(flow, 1000);
    }

    std::cout << "  50 个轻流 (各 20 包)" << std::endl;
    for (int i = 0; i < 50; i++) {
        TwoTuple flow(ip_to_uint32("192.168.2.1") + i,
                      ip_to_uint32("10.0.0.2"));
        flows.push_back({flow, 20});
        es.update(flow, 20);
    }

    // 检测重流（阈值：500）
    std::cout << "\n检测重流（阈值：500 包）...\n" << std::endl;

    int detected = 0;
    for (const auto& f : flows) {
        uint64_t count = es.query(f.first);
        if (count >= 500) {
            std::cout << "  检测到重流: " << uint32_to_ip(f.first.src_ip)
                      << " → " << uint32_to_ip(f.first.dst_ip) << " (" << count
                      << " 包)" << std::endl;
            detected++;
        }
    }

    std::cout << "\n结果: 检测到 " << detected << "/5 个重流" << std::endl;
    std::cout << "特点: 自动识别大流，无需预先指定" << std::endl;
}

void example_elasticsketch_lambda_comparison() {
    print_separator("示例 16: ElasticSketch Lambda 参数影响");

    // 不同的 lambda 值
    ElasticSketch es_aggressive(1000, 1, 4096);     // Lambda=1, 激进替换
    ElasticSketch es_moderate(1000, 3, 4096);       // Lambda=3, 适中
    ElasticSketch es_conservative(1000, 10, 4096);  // Lambda=10, 保守

    std::cout << "\n配置: 相同内存，不同 Lambda 值" << std::endl;
    std::cout << "  激进 (λ=1): 容易替换，适应快" << std::endl;
    std::cout << "  适中 (λ=3): 平衡替换" << std::endl;
    std::cout << "  保守 (λ=10): 不易替换，稳定" << std::endl;

    // 测试流
    std::vector<TwoTuple> test_flows;
    for (int i = 0; i < 20; i++) {
        test_flows.emplace_back(ip_to_uint32("192.168.1.1") + i * 0x100000,
                                ip_to_uint32("10.0.0.1"));
    }

    std::cout << "\n插入 20 个流，各 50 包..." << std::endl;
    for (const auto& flow : test_flows) {
        es_aggressive.update(flow, 50);
        es_moderate.update(flow, 50);
        es_conservative.update(flow, 50);
    }

    // 统计准确率
    int correct_aggressive = 0, correct_moderate = 0, correct_conservative = 0;
    for (const auto& flow : test_flows) {
        if (es_aggressive.query(flow) >= 45)
            correct_aggressive++;
        if (es_moderate.query(flow) >= 45)
            correct_moderate++;
        if (es_conservative.query(flow) >= 45)
            correct_conservative++;
    }

    std::cout << "\n准确率（允许 10% 误差）:" << std::endl;
    std::cout << "  激进 (λ=1): " << correct_aggressive << "/20 ("
              << (100.0 * correct_aggressive / 20) << "%)" << std::endl;
    std::cout << "  适中 (λ=3): " << correct_moderate << "/20 ("
              << (100.0 * correct_moderate / 20) << "%)" << std::endl;
    std::cout << "  保守 (λ=10): " << correct_conservative << "/20 ("
              << (100.0 * correct_conservative / 20) << "%)" << std::endl;

    std::cout << "\n结论: Lambda 值越大，越稳定但适应性越差" << std::endl;
}

void example_real_world_scenario() {
    print_separator("示例 17: 实际应用场景 - 网络流量监控");

    std::cout << "\n场景: 监控网络流量，检测 DDoS 攻击" << std::endl;
    std::cout << "策略: 使用 ElasticSketch 自适应检测异常流量\n" << std::endl;

    // 创建 ElasticSketch
    ElasticSketch es(8000, 2, 32768, 8);

    // 模拟正常流量（分散）
    std::cout << "1. 添加正常流量..." << std::endl;
    for (int i = 0; i < 100; i++) {
        TwoTuple flow(ip_to_uint32("192.168.1.1") + i,
                      ip_to_uint32("10.0.0.1"));
        es.update(flow, 10);  // 每个流 10 个包
    }

    // 模拟 DDoS 攻击（单个 IP 发送大量请求）
    std::cout << "2. 检测到异常流量..." << std::endl;
    TwoTuple attacker(ip_to_uint32("203.0.113.1"), ip_to_uint32("10.0.0.1"));
    es.update(attacker, 10000);  // 攻击者发送 10000 个包

    // 查询并检测
    std::cout << "\n3. 分析结果:" << std::endl;
    uint64_t attacker_count = es.query(attacker);

    if (attacker_count > 1000) {
        std::cout << "  ⚠️  检测到 DDoS 攻击！" << std::endl;
        std::cout << "  攻击源: " << uint32_to_ip(attacker.src_ip) << std::endl;
        std::cout << "  数据包数: " << attacker_count << std::endl;
        std::cout << "  建议: 立即阻断该 IP" << std::endl;
    } else {
        std::cout << "  ✓ 流量正常" << std::endl;
    }

    std::cout << "\n优势: ElasticSketch 自动将攻击者置于 Heavy Part，"
              << "提供精确计数" << std::endl;
}

// ==================== 主函数 ====================

int main() {
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════════════"
                 "════════╗\n";
    std::cout << "║                 SketchLib 完整使用示例                     "
                 "         ║\n";
    std::cout << "║                                                            "
                 "        ║\n";
    std::cout << "║  包含所有 Sketch 算法的演示：                              "
                 "         ║\n";
    std::cout << "║  - Count-Min Sketch     (频率估计)                         "
                 "       ║\n";
    std::cout << "║  - Count Sketch         (无偏估计)                         "
                 "       ║\n";
    std::cout << "║  - Sample-and-Hold      (精确追踪)                         "
                 "       ║\n";
    std::cout << "║  - UnivMon              (多分辨率监控)                     "
                 "        ║\n";
    std::cout << "║  - ElasticSketch        (自适应大流识别)                   "
                 "        ║\n";
    std::cout << "║  - 自定义哈希函数       (依赖注入)                         "
                 "        ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════"
                 "════════╝\n";

    try {
        // Count-Min Sketch 示例
        example_countmin_basic();
        example_countmin_heavy_hitter();

        // Count Sketch 示例
        example_countsketch_basic();
        example_countsketch_vs_countmin();

        // Sample-and-Hold 示例
        example_sampleandhold_basic();
        example_sampleandhold_eviction();
        example_sampleandhold_heavy_hitter();

        // UnivMon 示例
        example_univmon_basic();
        example_univmon_multi_scale();
        example_univmon_backend_comparison();

        // ElasticSketch 示例
        example_elasticsketch_basic();
        example_elasticsketch_eviction();
        example_elasticsketch_heavy_hitter();
        example_elasticsketch_lambda_comparison();

        // 自定义哈希函数示例
        example_custom_hash();
        example_hash_comparison();

        // 综合示例
        example_real_world_scenario();

        print_separator("所有示例运行完成！");
        std::cout << "\n更多信息请查看 README.md 文档" << std::endl;
        std::cout << "\n";

    } catch (const std::exception& e) {
        std::cerr << "\n错误: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
