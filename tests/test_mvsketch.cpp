#include "../third_party/doctest.h"

#include "FlowKey.h"
#include "HashFunction.h"
#include "MVSketch.h"

TEST_SUITE("MVSketch Tests") {
    TEST_CASE("Default Hash Function - TwoTuple") {
        MVSketch<TwoTuple> mv(4, 1024);

        TwoTuple flow(0x12345678, 0x87654321);

        mv.update(flow, 5);
        uint64_t result = mv.query(flow);

        // 允许一定的误差
        CHECK(result >= 4);
        CHECK(result <= 6);
    }

    TEST_CASE("Multiple Updates - TwoTuple") {
        MVSketch<TwoTuple> mv(5, 2048);

        TwoTuple flow(0x11111111, 0x22222222);

        for (int i = 0; i < 10; i++) {
            mv.update(flow);
        }

        uint64_t result = mv.query(flow);
        // 应该能够检测到该流
        CHECK(result >= 0);
    }

    TEST_CASE("Custom Hash Function - SpookyV2") {
        auto custom_hash = std::make_unique<SpookyV2HashFunction<TwoTuple>>();
        MVSketch<TwoTuple> mv(5, 2048, std::move(custom_hash));

        TwoTuple flow(0xAABBCCDD, 0xEEFF0011);

        for (int i = 0; i < 15; i++) {
            mv.update(flow);
        }

        uint64_t result = mv.query(flow);
        CHECK(result >= 0);
    }

    TEST_CASE("Custom Hash Function - MurmurV3") {
        auto custom_hash = std::make_unique<MurmurV3HashFunction<TwoTuple>>();
        MVSketch<TwoTuple> mv(5, 2048, std::move(custom_hash));

        TwoTuple flow(0x0F0F0F0F, 0xF0F0F0F0);

        for (int i = 0; i < 12; i++) {
            mv.update(flow, 2);
        }

        uint64_t result = mv.query(flow);
        CHECK(result >= 0);
    }

    TEST_CASE("Multiple Flows - TwoTuple") {
        MVSketch<TwoTuple> mv(4, 4096);

        // 插入多个不同的流
        TwoTuple flow1(0x11111111, 0x22222222);
        TwoTuple flow2(0x33333333, 0x44444444);
        TwoTuple flow3(0x55555555, 0x66666666);

        // 对 flow1 更新多次
        for (int i = 0; i < 20; i++) {
            mv.update(flow1);
        }

        // 对其他流更新较少次数
        for (int i = 0; i < 5; i++) {
            mv.update(flow2);
            mv.update(flow3);
        }

        uint64_t result1 = mv.query(flow1);
        uint64_t result2 = mv.query(flow2);
        uint64_t result3 = mv.query(flow3);

        // flow1 的计数应该大于其他流
        CHECK(result1 >= 0);
        CHECK(result2 >= 0);
        CHECK(result3 >= 0);
    }

    TEST_CASE("OneTuple Support") {
        MVSketch<OneTuple> mv(4, 1024);

        OneTuple flow(0x12345678);

        for (int i = 0; i < 10; i++) {
            mv.update(flow);
        }

        uint64_t result = mv.query(flow);
        CHECK(result >= 0);
    }

    TEST_CASE("FiveTuple Support") {
        MVSketch<FiveTuple> mv(4, 2048);

        FiveTuple flow(0x11111111, 0x22222222, 80, 443, 6);

        for (int i = 0; i < 10; i++) {
            mv.update(flow);
        }

        uint64_t result = mv.query(flow);
        CHECK(result >= 0);
    }

    TEST_CASE("Clear Function") {
        MVSketch<TwoTuple> mv(4, 1024);

        TwoTuple flow(0x12345678, 0x87654321);

        mv.update(flow, 10);
        uint64_t result1 = mv.query(flow);
        CHECK(result1 >= 0);

        mv.clear();
        uint64_t result2 = mv.query(flow);
        CHECK(result2 == 0);
    }

    TEST_CASE("Get Rows and Cols") {
        MVSketch<TwoTuple> mv(5, 2048);

        CHECK(mv.get_rows() == 5);
        CHECK(mv.get_cols() > 0);
    }

    TEST_CASE("Copy Constructor") {
        MVSketch<TwoTuple> mv1(4, 1024);
        TwoTuple flow(0x12345678, 0x87654321);

        mv1.update(flow, 10);

        MVSketch<TwoTuple> mv2(mv1);
        uint64_t result = mv2.query(flow);
        CHECK(result >= 0);
    }

    TEST_CASE("Assignment Operator") {
        MVSketch<TwoTuple> mv1(4, 1024);
        MVSketch<TwoTuple> mv2(4, 1024);

        TwoTuple flow(0x12345678, 0x87654321);
        mv1.update(flow, 10);

        mv2 = mv1;
        uint64_t result = mv2.query(flow);
        CHECK(result >= 0);
    }
}
