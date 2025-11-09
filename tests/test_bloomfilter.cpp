#include "../third_party/doctest.h"

#include "BloomFilter.h"
#include "FlowKey.h"
#include "HashFunction.h"

TEST_SUITE("BloomFilter Tests") {
    TEST_CASE("Basic Insert and Query") {
        BloomFilter<TwoTuple> bf(1000, 3);

        TwoTuple flow(0x12345678, 0x87654321);

        // 插入前不存在
        CHECK(bf.query(flow) == 0);

        // 插入
        bf.update(flow);

        // 插入后存在
        CHECK(bf.query(flow) == 1);
    }

    TEST_CASE("Multiple Flows") {
        BloomFilter<TwoTuple> bf(10000, 4);

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
        auto custom_hash = std::make_unique<MurmurV3HashFunction<TwoTuple>>();
        BloomFilter<TwoTuple> bf(2000, 3, std::move(custom_hash));

        TwoTuple flow(0xAABBCCDD, 0xEEFF0011);

        bf.update(flow);
        CHECK(bf.query(flow) == 1);
    }

    TEST_CASE("False Positive Test") {
        // 小的 BF，容易产生假阳性
        BloomFilter<TwoTuple> bf(100, 2);

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
        BloomFilter<TwoTuple> bf(1000, 3);

        TwoTuple flow(0x12345678, 0x87654321);

        bf.update(flow);
        CHECK(bf.query(flow) == 1);

        bf.clear();

        CHECK(bf.query(flow) == 0);
    }

    TEST_CASE("Copy Constructor") {
        BloomFilter<TwoTuple> bf1(1000, 3);

        TwoTuple flow(0x12345678, 0x87654321);
        bf1.update(flow);

        BloomFilter<TwoTuple> bf2(bf1);

        CHECK(bf2.query(flow) == bf1.query(flow));
    }

    TEST_CASE("Get Parameters") {
        BloomFilter<TwoTuple> bf(8192, 3);
        auto num_bits = bf.get_num_bits();
        auto num_hashes = bf.get_num_hashes();
        CHECK(num_bits == 8192);
        CHECK(num_hashes == 3);
    }

    TEST_CASE("OneTuple Support") {
        BloomFilter<OneTuple> bf(4096, 3);
        OneTuple flow(0xDEADBEEF);

        bf.update(flow);
        uint64_t result = bf.query(flow);

        CHECK(result == 1);
    }

    TEST_CASE("FiveTuple Support") {
        BloomFilter<FiveTuple> bf(4096, 3);
        FiveTuple flow(0xCAFEBABE, 0xDEADBEEF, 443, 8080, 17);

        bf.update(flow);
        uint64_t result = bf.query(flow);

        CHECK(result == 1);
    }

}  // TEST_SUITE
