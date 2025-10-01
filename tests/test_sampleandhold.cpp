#include <cstdint>
#include <vector>
#include "../third_party/doctest.h"

#include "SampleAndHold.h"

TEST_SUITE("SampleAndHold Tests") {
    TEST_CASE("Basic Update") {
        SampleAndHold sah(10);

        TwoTuple flow(0x12345678, 0x87654321);

        sah.update(flow, 5);
        uint64_t result = sah.query(flow);

        CHECK(result == 5);
        CHECK(sah.has_flow(flow) == true);
        CHECK(sah.get_size() == 1);
        CHECK(sah.get_capacity() == 10);
    }

    TEST_CASE("Multiple Updates on Same Flow") {
        SampleAndHold sah(5);

        TwoTuple flow(0xAABBCCDD, 0x11223344);

        sah.update(flow, 10);
        sah.update(flow, 15);
        sah.update(flow, 5);

        uint64_t result = sah.query(flow);
        CHECK(result == 30);
        CHECK(sah.get_size() == 1);
    }

    TEST_CASE("Capacity Limit and Eviction Policy") {
        // 只能容纳3个流
        SampleAndHold sah(3);

        TwoTuple flow1(0x11111111, 0x22222222);
        TwoTuple flow2(0x33333333, 0x44444444);
        TwoTuple flow3(0x55555555, 0x66666666);
        TwoTuple flow4(0x77777777, 0x88888888);

        // 插入3个流
        sah.update(flow1, 10);
        sah.update(flow2, 20);
        sah.update(flow3, 5);

        CHECK(sah.get_size() == 3);
        CHECK(sah.query(flow1) == 10);
        CHECK(sah.query(flow2) == 20);
        CHECK(sah.query(flow3) == 5);

        // 插入第4个流，计数比最小的流大，应该驱逐flow3
        sah.update(flow4, 15);

        CHECK(sah.get_size() == 3);
        // flow3应该被驱逐
        CHECK(sah.has_flow(flow3) == false);
        CHECK(sah.query(flow3) == 0);
        CHECK(sah.query(flow4) == 15);
    }

    TEST_CASE("Validate Eviction") {
        SampleAndHold sah(2);

        TwoTuple flow1(0x11111111, 0x22222222);
        TwoTuple flow2(0x33333333, 0x44444444);
        TwoTuple flow3(0x55555555, 0x66666666);

        sah.update(flow1, 10);
        sah.update(flow2, 20);

        CHECK(sah.get_size() == 2);

        // 插入一个计数比最小值还小的流，不应该被接受
        sah.update(flow3, 5);

        CHECK(sah.get_size() == 2);
        CHECK(sah.has_flow(flow3) == false);
        CHECK(sah.has_flow(flow1) == true);
        CHECK(sah.has_flow(flow2) == true);
    }

    TEST_CASE("Query Non-Existent Flow") {
        SampleAndHold sah(5);

        TwoTuple flow1(0x12345678, 0x87654321);
        TwoTuple flow2(0xAABBCCDD, 0x11223344);

        sah.update(flow1, 100);

        CHECK(sah.query(flow2) == 0);
        CHECK(sah.has_flow(flow2) == false);
    }

    TEST_CASE("Zero Update") {
        SampleAndHold sah(5);

        TwoTuple flow(0x12345678, 0x87654321);

        sah.update(flow, 0);

        CHECK(sah.query(flow) == 0);
        CHECK(sah.get_size() == 0);
    }

    TEST_CASE("Negative Update") {
        SampleAndHold sah(5);

        TwoTuple flow(0x12345678, 0x87654321);

        sah.update(flow, -10);

        CHECK(sah.query(flow) == 0);
        CHECK(sah.get_size() == 0);
    }

    TEST_CASE("Stress Test with Heavy Flow") {
        SampleAndHold sah(100);

        TwoTuple heavy_flow(0x11111111, 0x22222222);

        // 模拟1000次更新
        for (int i = 0; i < 1000; i++) {
            sah.update(heavy_flow, 1);
        }

        // SaH应该精确记录
        CHECK(sah.query(heavy_flow) == 1000);
    }

    TEST_CASE("Multiple Flows Independence") {
        SampleAndHold sah(10);

        std::vector<TwoTuple> flows = {
            TwoTuple(0x11111111, 0x22222222), TwoTuple(0x33333333, 0x44444444),
            TwoTuple(0x55555555, 0x66666666), TwoTuple(0x77777777, 0x88888888)};

        std::vector<int> counts = {10, 20, 30, 40};

        for (size_t i = 0; i < flows.size(); i++) {
            sah.update(flows[i], counts[i]);
        }

        for (size_t i = 0; i < flows.size(); i++) {
            CHECK(sah.query(flows[i]) == static_cast<uint64_t>(counts[i]));
        }

        CHECK(sah.get_size() == flows.size());
    }

    TEST_CASE("Capacity Limit Edge Case") {
        SampleAndHold sah(1);

        TwoTuple flow1(0x11111111, 0x22222222);
        TwoTuple flow2(0x33333333, 0x44444444);

        sah.update(flow1, 10);
        CHECK(sah.query(flow1) == 10);
        CHECK(sah.get_size() == 1);

        // 插入更大的流，应该替换
        sah.update(flow2, 20);
        CHECK(sah.query(flow2) == 20);
        CHECK(sah.query(flow1) == 0);
        CHECK(sah.get_size() == 1);
    }

    TEST_CASE("Large Capacity Test") {
        SampleAndHold sah(1000);

        // 插入500个流
        for (uint32_t i = 0; i < 500; i++) {
            TwoTuple flow(i, i + 1000);
            sah.update(flow, i + 1);
        }

        CHECK(sah.get_size() == 500);

        // 验证所有流
        for (uint32_t i = 0; i < 500; i++) {
            TwoTuple flow(i, i + 1000);
            CHECK(sah.query(flow) == static_cast<uint64_t>(i + 1));
        }
    }

}  // TEST_SUITE
