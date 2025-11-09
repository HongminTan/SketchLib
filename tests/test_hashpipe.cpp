#include "../third_party/doctest.h"

#include <vector>

#include "FlowKey.h"
#include "HashFunction.h"
#include "HashPipe.h"

TEST_SUITE("HashPipe Tests") {
    TEST_CASE("Basic Insert and Query") {
        HashPipe<TwoTuple> hp(4096);

        TwoTuple flow(0x12345678, 0x87654321);

        hp.update(flow, 5);
        uint64_t result = hp.query(flow);

        CHECK(result == 5);
    }

    TEST_CASE("Heavy Flow Detection") {
        HashPipe<TwoTuple> hp(8192);

        TwoTuple heavy_flow(0x11111111, 0x22222222);
        TwoTuple light_flow(0x33333333, 0x44444444);

        // 插入大流
        hp.update(heavy_flow, 1000);
        // 插入小流
        hp.update(light_flow, 10);

        uint64_t heavy_count = hp.query(heavy_flow);
        uint64_t light_count = hp.query(light_flow);

        // 大流应该被准确记录
        CHECK(heavy_count == 1000);
        // 小流可能被过滤（返回 0）或记录
        CHECK(light_count <= 10);
    }

    TEST_CASE("Multiple Heavy Flows") {
        HashPipe<TwoTuple> hp(16384, 8);

        std::vector<TwoTuple> flows;
        for (int i = 0; i < 5; i++) {
            flows.emplace_back(0x10000000 + i * 0x1000000,
                               0x20000000 + i * 0x2000000);
        }

        // 插入多个大流
        for (const auto& flow : flows) {
            hp.update(flow, 500);
        }

        // 验证所有大流都能查询到
        int found = 0;
        for (const auto& flow : flows) {
            uint64_t count = hp.query(flow);
            if (count == 500) {
                found++;
            }
        }

        CHECK(found == 5);
    }

    TEST_CASE("Custom Hash Function - MurmurV3") {
        auto custom_hash = std::make_unique<MurmurV3HashFunction<TwoTuple>>();
        HashPipe<TwoTuple> hp(4096, 8, std::move(custom_hash));

        TwoTuple flow(0xAABBCCDD, 0xEEFF0011);

        hp.update(flow, 15);
        uint64_t result = hp.query(flow);

        CHECK(result >= 15);
    }

    TEST_CASE("Custom Hash Function - SpookyV2") {
        auto custom_hash = std::make_unique<SpookyV2HashFunction<TwoTuple>>();
        HashPipe<TwoTuple> hp(4096, 8, std::move(custom_hash));

        TwoTuple flow(0x12341234, 0x56785678);

        hp.update(flow, 25);
        uint64_t result = hp.query(flow);

        CHECK(result >= 25);
    }

    TEST_CASE("Custom Hash Function - CRC64") {
        auto custom_hash = std::make_unique<CRC64HashFunction<TwoTuple>>();
        HashPipe<TwoTuple> hp(4096, 8, std::move(custom_hash));

        TwoTuple flow(0x0F0F0F0F, 0xF0F0F0F0);

        hp.update(flow, 20);
        uint64_t result = hp.query(flow);

        CHECK(result >= 20);
    }

    TEST_CASE("Query Non-Existent Flow") {
        HashPipe<TwoTuple> hp(4096);

        TwoTuple flow(0x00000000, 0x00000001);
        uint64_t result = hp.query(flow);

        CHECK(result == 0);

        bool exists = hp.has_flow(flow);
        CHECK(!exists);
    }

    TEST_CASE("Clear Functionality") {
        HashPipe<TwoTuple> hp(4096);

        TwoTuple flow1(0x11111111, 0x22222222);
        TwoTuple flow2(0x33333333, 0x44444444);

        hp.update(flow1, 100);
        hp.update(flow2, 200);

        hp.clear();

        CHECK(hp.query(flow1) == 0);
        CHECK(hp.query(flow2) == 0);
    }

    TEST_CASE("OneTuple Support") {
        HashPipe<OneTuple> hp(4096, 4);
        OneTuple flow(0xFEDCBA98);

        hp.update(flow, 25);
        uint64_t result = hp.query(flow);

        CHECK(result == 25);
    }

    TEST_CASE("FiveTuple Support") {
        HashPipe<FiveTuple> hp(4096, 4);
        FiveTuple flow(0x11223344, 0x55667788, 3000, 8000, 17);

        hp.update(flow, 35);
        uint64_t result = hp.query(flow);

        CHECK(result == 35);
    }

    TEST_CASE("Different Stage Numbers") {
        HashPipe<TwoTuple> hp_4stages(8192, 4);
        HashPipe<TwoTuple> hp_16stages(8192, 16);

        CHECK(hp_4stages.get_num_stages() == 4);
        CHECK(hp_16stages.get_num_stages() == 16);

        TwoTuple flow(0xAAAABBBB, 0xCCCCDDDD);

        hp_4stages.update(flow, 100);
        hp_16stages.update(flow, 100);

        uint64_t count_4 = hp_4stages.query(flow);
        uint64_t count_16 = hp_16stages.query(flow);

        CHECK(count_4 >= 90);
        CHECK(count_16 >= 90);
    }

    TEST_CASE("Pipeline Eviction - Light Flows Filtered") {
        HashPipe<TwoTuple> hp(4096, 8);  // 小内存，容易触发替换

        // 先插入一个大流
        TwoTuple heavy_flow(0x99999999, 0x88888888);
        hp.update(heavy_flow, 1000);

        // 然后插入很多小流，可能触发替换
        for (int i = 0; i < 100; i++) {
            TwoTuple light_flow(0x10000000 + i, 0x20000000 + i);
            hp.update(light_flow, 5);
        }

        // 大流应该仍然存在
        uint64_t heavy_count = hp.query(heavy_flow);
        CHECK(heavy_count == 1000);
    }

    TEST_CASE("Stress Test - Many Flows") {
        HashPipe<TwoTuple> hp(16384, 8);

        // 插入大量不同的流
        std::vector<TwoTuple> flows;
        for (int i = 0; i < 50; i++) {
            flows.emplace_back(i * 12345, i * 67890);
        }

        // 按不同的频率更新
        for (size_t i = 0; i < flows.size(); i++) {
            int count = (i < 10) ? 100 : 5;  // 前 10 个是大流
            hp.update(flows[i], count);
        }

        // 验证大流能被找到
        int heavy_found = 0;
        for (size_t i = 0; i < 10; i++) {
            uint64_t result = hp.query(flows[i]);
            if (result == 100) {
                heavy_found++;
            }
        }

        CHECK(heavy_found == 10);
    }

    TEST_CASE("Copy Constructor") {
        HashPipe<TwoTuple> hp1(4096);

        TwoTuple flow(0x12345678, 0x87654321);
        hp1.update(flow, 10);

        HashPipe<TwoTuple> hp2(hp1);

        CHECK(hp2.query(flow) == hp1.query(flow));

        hp2.update(flow, 5);
        CHECK(hp2.query(flow) != hp1.query(flow));
    }

    TEST_CASE("Assignment Operator") {
        HashPipe<TwoTuple> hp1(4096);
        HashPipe<TwoTuple> hp2(8192);

        TwoTuple flow(0xAAAABBBB, 0xCCCCDDDD);
        hp1.update(flow, 15);

        hp2 = hp1;

        CHECK(hp2.query(flow) == hp1.query(flow));
        CHECK(hp2.get_num_stages() == hp1.get_num_stages());
    }

    TEST_CASE("Get Parameters") {
        HashPipe<TwoTuple> hp(8192, 10);

        CHECK(hp.get_num_stages() == 10);
        CHECK(hp.get_buckets_per_stage() ==
              8192 / 10 / sizeof(HPBucket<TwoTuple>));
    }

    TEST_CASE("Incremental Updates") {
        HashPipe<TwoTuple> hp(4096);

        TwoTuple flow(0x11223344, 0x55667788);

        for (int i = 0; i < 20; i++) {
            hp.update(flow, 1);
        }

        uint64_t result = hp.query(flow);
        CHECK(result >= 20);
    }

}  // TEST_SUITE
