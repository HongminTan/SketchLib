#include "../third_party/doctest.h"

#include "BloomFilter.h"
#include "HashFunction.h"

TEST_SUITE("BloomFilter Tests") {
    TEST_CASE("Basic Insert and Query") {
        BloomFilter bf(1000, 3);

        TwoTuple flow(0x12345678, 0x87654321);

        // 插入前不存在
        CHECK(bf.query(flow) == 0);

        // 插入
        bf.update(flow);

        // 插入后存在
        CHECK(bf.query(flow) == 1);
    }

    TEST_CASE("Multiple Flows") {
        BloomFilter bf(10000, 4);

        std::vector<TwoTuple> flows;
        for (int i = 0; i < 10; i++) {
            flows.emplace_back(i * 1000, i * 2000);
        }

        // 插入所有流
        for (const auto& flow : flows) {
            bf.update(flow);
        }

        // 查询所有流应该存在
        for (const auto& flow : flows) {
            CHECK(bf.query(flow) == 1);
        }
    }

    TEST_CASE("Custom Hash Function - MurmurV3") {
        auto custom_hash = std::make_unique<MurmurV3HashFunction>();
        BloomFilter bf(2000, 3, std::move(custom_hash));

        TwoTuple flow(0xAABBCCDD, 0xEEFF0011);

        bf.update(flow);
        CHECK(bf.query(flow) == 1);
    }

    TEST_CASE("False Positive Test") {
        BloomFilter bf(100, 2);  // 小的 BF，容易产生假阳性

        TwoTuple flow1(0x11111111, 0x22222222);
        TwoTuple flow2(0x33333333, 0x44444444);

        bf.update(flow1);

        // flow1 应该存在
        CHECK(bf.query(flow1) == 1);

        // flow2 可能存在或不存在
        uint64_t result = bf.query(flow2);
        CHECK((result == 0 || result == 1));
    }

    TEST_CASE("Clear Functionality") {
        BloomFilter bf(1000, 3);

        TwoTuple flow(0x12345678, 0x87654321);

        bf.update(flow);
        CHECK(bf.query(flow) == 1);

        bf.clear();

        CHECK(bf.query(flow) == 0);
    }

    TEST_CASE("Copy Constructor") {
        BloomFilter bf1(1000, 3);

        TwoTuple flow(0x12345678, 0x87654321);
        bf1.update(flow);

        BloomFilter bf2(bf1);

        CHECK(bf2.query(flow) == bf1.query(flow));
    }

    TEST_CASE("Get Parameters") {
        BloomFilter bf(5000, 7);

        CHECK(bf.get_num_bits() == 5000);
        CHECK(bf.get_num_hashes() == 7);
    }
}  // TEST_SUITE
