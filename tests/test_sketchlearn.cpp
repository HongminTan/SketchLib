#include "../third_party/doctest.h"

#include <map>

#include "HashFunction.h"
#include "SketchLearn.h"

TEST_SUITE("SketchLearn Tests") {
    TEST_CASE("Basic Update and Decode") {
        SketchLearn sl(16384, 1);

        TwoTuple flow1(0x00001111, 0x00002222);
        TwoTuple flow2(0x00003333, 0x00004444);

        sl.update(flow1, 10);
        sl.update(flow2, 20);

        // 执行解码
        auto result = sl.decode();

        CHECK(result.size() == 2);
    }

    TEST_CASE("Decode Returns Map") {
        SketchLearn sl(16384, 1);

        TwoTuple flow(0x00001111, 0x00002222);
        sl.update(flow, 50);

        // 执行解码
        auto result = sl.decode();

        // 解码成功
        CHECK(result.size() == 1);
    }

    TEST_CASE("Different Theta Values") {
        SketchLearn sl_low(8192, 1, 0.3);   // 低阈值
        SketchLearn sl_high(8192, 1, 0.7);  // 高阈值

        CHECK(sl_low.get_theta() == 0.3);
        CHECK(sl_high.get_theta() == 0.7);
    }

    TEST_CASE("Different Row Numbers") {
        SketchLearn sl_1row(8192, 1);    // 1 行
        SketchLearn sl_3rows(16384, 3);  // 3 行

        CHECK(sl_1row.get_num_rows() == 1);
        CHECK(sl_3rows.get_num_rows() == 3);
        CHECK(sl_1row.get_num_bits() == 64);
        CHECK(sl_3rows.get_num_bits() == 64);
    }

    TEST_CASE("Custom Hash Function - MurmurV3") {
        auto custom_hash = std::make_unique<MurmurV3HashFunction>();
        SketchLearn sl(8192, 1, 0.5, std::move(custom_hash));

        TwoTuple flow(0x0000CCDD, 0x00000011);
        sl.update(flow, 25);

        uint64_t result = sl.query(flow);
        CHECK(result == 25);
    }

    TEST_CASE("Query Non-Existent Flow") {
        SketchLearn sl(8192, 1);

        TwoTuple flow1(0x11111111, 0x22222222);
        TwoTuple flow2(0x33333333, 0x44444444);

        sl.update(flow1, 10);

        // 不存在的流
        uint64_t count = sl.query(flow2);
        CHECK(count == 0);
    }

    TEST_CASE("Clear Functionality") {
        SketchLearn sl(8192, 1);

        TwoTuple flow(0x11111111, 0x22222222);
        sl.update(flow, 10);

        sl.clear();

        CHECK(sl.query(flow) == 0);
    }

    TEST_CASE("Multiple Updates") {
        SketchLearn sl(8192, 1);

        TwoTuple flow(0x00001122, 0x00003344);

        // 逐步更新
        for (int i = 0; i < 5; i++) {
            sl.update(flow, 2);
        }

        auto result = sl.decode();
        CHECK(result.size() == 1);
        CHECK(sl.query(flow) == 10);
    }

    TEST_CASE("Copy Constructor") {
        SketchLearn sl1(8192, 1);

        TwoTuple flow(0x12345678, 0x87654321);
        sl1.update(flow, 10);
        sl1.decode();

        SketchLearn sl2(sl1);

        CHECK(sl2.query(flow) == sl1.query(flow));
    }

    TEST_CASE("Get Parameters") {
        SketchLearn sl(8192, 2, 0.6);

        CHECK(sl.get_num_bits() == 64);
        CHECK(sl.get_num_rows() == 2);
        CHECK(sl.get_num_cols() > 0);
        CHECK(sl.get_theta() == 0.6);
    }

    TEST_CASE("Multi Row CountMin") {
        SketchLearn sl(16384, 3);  // 3 行 CountMin

        TwoTuple flow(0x00000000, 0x000000FF);
        sl.update(flow, 10);

        auto result = sl.decode();
        CHECK(result.size() == 1);
        CHECK(sl.query(flow) == 10);
    }

}  // TEST_SUITE
