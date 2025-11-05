#include "../third_party/doctest.h"

#include "ElasticSketch.h"
#include "HashFunction.h"

TEST_SUITE("ElasticSketch Tests") {
    TEST_CASE("Basic Insert and Query") {
        ElasticSketch es(400, 2, 1024);

        TwoTuple flow(0x12345678, 0x87654321);

        es.update(flow, 5);
        uint64_t result = es.query(flow);

        CHECK(result >= 5);
    }

    TEST_CASE("Heavy Part Exact Counting") {
        ElasticSketch es(1000, 3, 2048);

        TwoTuple flow1(0x11111111, 0x22222222);
        TwoTuple flow2(0x33333333, 0x44444444);
        TwoTuple flow3(0x55555555, 0x66666666);

        es.update(flow1, 10);
        es.update(flow2, 20);
        es.update(flow3, 30);

        uint64_t count1 = es.query(flow1);
        uint64_t count2 = es.query(flow2);
        uint64_t count3 = es.query(flow3);

        CHECK(count1 >= 10);
        CHECK(count2 >= 20);
        CHECK(count3 >= 30);
    }

    TEST_CASE("Eviction Mechanism") {
        ElasticSketch es(100, 2, 1024);

        TwoTuple flow1(0xAAAAAAAA, 0xBBBBBBBB);
        TwoTuple flow2(0xCCCCCCCC, 0xDDDDDDDD);

        es.update(flow1, 100);

        for (int i = 0; i < 50; i++) {
            es.update(flow2, 1);
        }

        uint64_t count1 = es.query(flow1);
        uint64_t count2 = es.query(flow2);

        CHECK(count1 >= 100);  // 可能被部分踢出
        CHECK(count2 >= 50);   // 近似计数
    }

    TEST_CASE("Custom Hash Function - MurmurV3") {
        auto custom_hash = std::make_unique<MurmurV3HashFunction>();
        ElasticSketch es(500, 2, 2048, 8, std::move(custom_hash));

        TwoTuple flow(0xAABBCCDD, 0xEEFF0011);

        for (int i = 0; i < 15; i++) {
            es.update(flow);
        }

        uint64_t result = es.query(flow);
        CHECK(result >= 15);
    }

    TEST_CASE("Custom Hash Function - SpookyV2") {
        auto custom_hash = std::make_unique<SpookyV2HashFunction>();
        ElasticSketch es(500, 3, 2048, 8, std::move(custom_hash));

        TwoTuple flow(0x12341234, 0x56785678);

        es.update(flow, 25);

        uint64_t result = es.query(flow);
        CHECK(result >= 25);
    }

    TEST_CASE("Custom Hash Function - CRC64") {
        auto custom_hash = std::make_unique<CRC64HashFunction>();
        ElasticSketch es(400, 2, 1536, 8, std::move(custom_hash));

        TwoTuple flow(0x0F0F0F0F, 0xF0F0F0F0);

        for (int i = 0; i < 12; i++) {
            es.update(flow, 2);
        }

        uint64_t result = es.query(flow);
        CHECK(result >= 24);
    }

    TEST_CASE("Multiple Flows with Different Lambdas") {
        ElasticSketch es_low(1000, 1, 4096);
        ElasticSketch es_high(1000, 10, 4096);

        std::vector<TwoTuple> flows;
        for (int i = 0; i < 5; i++) {
            flows.emplace_back(0x10000000 + i * 0x1000000,
                               0x20000000 + i * 0x2000000);
        }

        for (const auto& flow : flows) {
            es_low.update(flow, 10);
            es_high.update(flow, 10);
        }

        int correct_low = 0;
        int correct_high = 0;
        for (const auto& flow : flows) {
            uint64_t count_low = es_low.query(flow);
            uint64_t count_high = es_high.query(flow);

            if (count_low >= 8)
                correct_low++;
            if (count_high >= 9)
                correct_high++;
        }

        CHECK(correct_low >= 3);
        CHECK(correct_high >= 4);
    }

    TEST_CASE("Query Non-Existent Flow") {
        ElasticSketch es(400, 2, 1024);

        TwoTuple flow(0x00000000, 0x00000001);
        uint64_t result = es.query(flow);

        CHECK(result == 0);

        bool exists = es.has_flow(flow);
        CHECK(!exists);
    }

    TEST_CASE("Clear Functionality") {
        ElasticSketch es(400, 2, 1024);

        TwoTuple flow1(0x11111111, 0x22222222);
        TwoTuple flow2(0x33333333, 0x44444444);

        es.update(flow1, 10);
        es.update(flow2, 20);

        CHECK(es.query(flow1) >= 10);
        CHECK(es.query(flow2) >= 20);

        es.clear();

        CHECK(es.query(flow1) == 0);
        CHECK(es.query(flow2) == 0);
    }

    TEST_CASE("Stress Test - Many Flows") {
        ElasticSketch es(2000, 2, 8192);

        std::vector<TwoTuple> flows;
        for (int i = 0; i < 100; i++) {
            flows.emplace_back(i * 12345, i * 67890);
        }

        for (size_t i = 0; i < flows.size(); i++) {
            es.update(flows[i], static_cast<int>(i + 1));
        }

        int correct_count = 0;
        for (size_t i = 0; i < flows.size(); i++) {
            uint64_t result = es.query(flows[i]);
            if (result >= i + 1) {
                correct_count++;
            }
        }

        CHECK(correct_count >= 50);
    }

    TEST_CASE("Stress Test - High Frequency Flow") {
        ElasticSketch es(1000, 2, 4096);

        TwoTuple heavy_flow(0x99999999, 0x88888888);

        uint32_t large_count = 100000;
        es.update(heavy_flow, large_count);

        uint64_t result = es.query(heavy_flow);

        // 允许5%误差
        CHECK(result >= large_count * 0.95);
    }

    TEST_CASE("Get Parameters") {
        ElasticSketch es(1000, 3, 4096, 10);

        uint64_t bucket_count = es.get_heavy_bucket_count();
        auto light_size = es.get_light_size();
        uint64_t lambda = es.get_lambda();

        CHECK(bucket_count == 1000 / HEAVY_BUCKET_SIZE);
        CHECK(light_size.first == 10);
        CHECK(light_size.second == (4096 - 1000) / 10 / 4);
        CHECK(lambda == 3);
    }

    TEST_CASE("Light Part Rows Parameter") {
        ElasticSketch es_4rows(500, 2, 2048, 4);
        ElasticSketch es_16rows(500, 2, 2048, 16);

        auto size_4 = es_4rows.get_light_size();
        auto size_16 = es_16rows.get_light_size();

        CHECK(size_4.first == 4);
        CHECK(size_16.first == 16);

        CHECK(size_4.second > size_16.second);
    }

    TEST_CASE("Copy Constructor") {
        ElasticSketch es1(400, 2, 1024);

        TwoTuple flow(0x12345678, 0x87654321);
        es1.update(flow, 10);

        ElasticSketch es2(es1);

        CHECK(es2.query(flow) == es1.query(flow));

        es2.update(flow, 5);
        CHECK(es2.query(flow) != es1.query(flow));
    }

    TEST_CASE("Assignment Operator") {
        ElasticSketch es1(400, 2, 1024);
        ElasticSketch es2(600, 3, 2048);

        TwoTuple flow(0xAAAABBBB, 0xCCCCDDDD);
        es1.update(flow, 15);

        es2 = es1;

        CHECK(es2.query(flow) == es1.query(flow));
        CHECK(es2.get_lambda() == es1.get_lambda());
    }

    TEST_CASE("Incremental Updates") {
        ElasticSketch es(500, 2, 2048);

        TwoTuple flow(0x11223344, 0x55667788);

        for (int i = 0; i < 20; i++) {
            es.update(flow, 1);
        }

        uint64_t result = es.query(flow);
        CHECK(result >= 20);
    }

}  // TEST_SUITE
