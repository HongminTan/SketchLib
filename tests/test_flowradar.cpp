#include "../third_party/doctest.h"

#include <map>

#include "FlowKey.h"
#include "FlowRadar.h"
#include "HashFunction.h"

TEST_SUITE("FlowRadar Tests") {
    TEST_CASE("Basic Update and Decode") {
        FlowRadar<TwoTuple> fr(4096);

        TwoTuple flow1(0x11111111, 0x22222222);
        TwoTuple flow2(0x33333333, 0x44444444);

        fr.update(flow1, 10);
        fr.update(flow2, 20);

        auto result = fr.decode();

        // 内存足以解码出这两个流
        CHECK(result.size() == 2);

        // 查询
        CHECK(fr.query(flow1) == 10);
        CHECK(fr.query(flow2) == 20);
    }

    TEST_CASE("Multiple Flows Decode") {
        FlowRadar<TwoTuple> fr(16384);

        std::vector<std::pair<TwoTuple, int>> flows;
        for (int i = 0; i < 5; i++) {
            TwoTuple flow(0x10000000 + i * 0x1000000,
                          0x20000000 + i * 0x2000000);
            flows.push_back({flow, (i + 1) * 10});
        }

        // 插入所有流
        for (const auto& f : flows) {
            fr.update(f.first, f.second);
        }

        // 解码
        auto result = fr.decode();

        // 验证解码结果
        int decoded_count = 0;
        for (const auto& f : flows) {
            if (fr.query(f.first) == static_cast<uint64_t>(f.second)) {
                decoded_count++;
            }
        }

        // 应该解码出所有流
        CHECK(decoded_count == 5);
    }

    TEST_CASE("Query Before Decode") {
        FlowRadar<TwoTuple> fr(4096);

        TwoTuple flow(0x12345678, 0x87654321);
        fr.update(flow, 15);

        // query 会自动调用 decode
        uint64_t count = fr.query(flow);
        CHECK(count == 15);
    }

    TEST_CASE("Query Non-Existent Flow") {
        FlowRadar<TwoTuple> fr(4096);

        TwoTuple flow1(0x11111111, 0x22222222);
        TwoTuple flow2(0x33333333, 0x44444444);

        fr.update(flow1, 10);

        CHECK(fr.query(flow2) == 0);
    }

    TEST_CASE("Custom Hash Function - MurmurV3") {
        auto custom_hash = std::make_unique<MurmurV3HashFunction<TwoTuple>>();
        FlowRadar<TwoTuple> fr(8192, 0.3, 3, 3, std::move(custom_hash));

        TwoTuple flow(0xAABBCCDD, 0xEEFF0011);

        fr.update(flow, 25);

        CHECK(fr.query(flow) == 25);
    }

    TEST_CASE("Different BF Percentage") {
        // 20% BF
        FlowRadar<TwoTuple> fr1(8192, 0.2);
        // 50% BF
        FlowRadar<TwoTuple> fr2(8192, 0.5);

        TwoTuple flow(0x12345678, 0x87654321);

        fr1.update(flow, 10);
        fr2.update(flow, 10);

        CHECK(fr1.query(flow) == 10);
        CHECK(fr2.query(flow) == 10);
    }

    TEST_CASE("Clear Functionality") {
        FlowRadar<TwoTuple> fr(4096);

        TwoTuple flow(0x11111111, 0x22222222);
        fr.update(flow, 10);

        CHECK(fr.query(flow) == 10);

        fr.clear();

        CHECK(fr.query(flow) == 0);
    }

    TEST_CASE("OneTuple Support") {
        FlowRadar<OneTuple> fr(4096);
        OneTuple flow(0x88776655);

        fr.update(flow, 15);
        uint64_t result = fr.query(flow);

        CHECK(result == 15);
    }

    TEST_CASE("FiveTuple Support") {
        FlowRadar<FiveTuple> fr(4096);
        FiveTuple flow(0xDEADBEEF, 0xCAFEBABE, 8080, 443, 6);

        fr.update(flow, 20);
        uint64_t result = fr.query(flow);

        CHECK(result == 20);
    }

    TEST_CASE("Incremental Updates") {
        FlowRadar<TwoTuple> fr(4096);

        TwoTuple flow(0x11111111, 0x22222222);

        for (int i = 0; i < 5; i++) {
            fr.update(flow, 2);
        }

        CHECK(fr.query(flow) == 10);
    }

    TEST_CASE("Decode Returns Map") {
        FlowRadar<TwoTuple> fr(8192);

        TwoTuple flow1(0x11111111, 0x22222222);
        TwoTuple flow2(0x33333333, 0x44444444);
        TwoTuple flow3(0x55555555, 0x66666666);

        fr.update(flow1, 10);
        fr.update(flow2, 20);
        fr.update(flow3, 30);

        auto result = fr.decode();

        CHECK(result.size() == 3);

        if (result.find(flow1) != result.end()) {
            CHECK(result[flow1] == 10);
        }
    }

    TEST_CASE("Stress Test - Many Flows") {
        FlowRadar<TwoTuple> fr(32768);

        std::vector<TwoTuple> flows;
        for (int i = 0; i < 20; i++) {
            flows.emplace_back(i * 12345, i * 67890);
        }

        for (size_t i = 0; i < flows.size(); i++) {
            fr.update(flows[i], (i + 1) * 5);
        }

        auto result = fr.decode();

        // 至少解码出 90% 的流
        CHECK(result.size() >= 18);
        CHECK(result.size() <= 20);
    }

    TEST_CASE("Copy Constructor") {
        FlowRadar<TwoTuple> fr1(4096);

        TwoTuple flow(0x12345678, 0x87654321);
        fr1.update(flow, 10);
        fr1.decode();

        FlowRadar<TwoTuple> fr2(fr1);

        CHECK(fr2.query(flow) == fr1.query(flow));
    }

    TEST_CASE("Get Parameters") {
        FlowRadar<TwoTuple> fr(8192, 0.3, 4, 5);

        CHECK(fr.get_bf_num_hashes() == 4);
        CHECK(fr.get_ct_num_hashes() == 5);
        CHECK(fr.get_table_size() > 0);
    }

}  // TEST_SUITE
