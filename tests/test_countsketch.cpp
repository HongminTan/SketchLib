#include "../third_party/doctest.h"

#include "HashFunction.h"
#include "CountSketch.h"

TEST_SUITE("CountSketch Tests") {
    TEST_CASE("Default Hash Function") {
        CountSketch cs(3, 1024);

        TwoTuple flow(0x12345678, 0x87654321);

        cs.update(flow, 5);
        uint64_t result = cs.query(flow);

        CHECK(result >= 5);
    }

    TEST_CASE("Multiple Updates and Queries") {
        CountSketch cs(5, 2048);

        std::vector<std::pair<uint32_t, uint32_t>> flows = {
            {0x11111111, 0x22222222},
            {0x33333333, 0x44444444},
            {0x55555555, 0x66666666}};

        for (auto& f : flows) {
            TwoTuple flow(f.first, f.second);
            cs.update(flow, 10);
        }

        for (auto& f : flows) {
            TwoTuple flow(f.first, f.second);
            uint64_t result = cs.query(flow);
            // 允许一定的估计误差
            CHECK(result >= 8);
        }
    }

    TEST_CASE("Accumulative Updates for Same Flow") {
        CountSketch cs(4, 4096);

        TwoTuple flow(0xABCDEF00, 0x12345678);

        for (int i = 0; i < 100; i++) {
            cs.update(flow, 1);
        }

        uint64_t result = cs.query(flow);
        CHECK(result >= 90);
        CHECK(result <= 110);
    }

    TEST_CASE("Copy Constructor") {
        CountSketch cs1(3, 1024);

        TwoTuple flow(0xDEADBEEF, 0xCAFEBABE);
        cs1.update(flow, 42);

        CountSketch cs2(cs1);

        uint64_t result1 = cs1.query(flow);
        uint64_t result2 = cs2.query(flow);

        CHECK(result1 == result2);
    }

    TEST_CASE("Custom Hash Function") {
        auto custom_hash = std::make_unique<SpookyV2HashFunction>();
        CountSketch cs(3, 1024, std::move(custom_hash));

        TwoTuple flow(0xAABBCCDD, 0x11223344);

        cs.update(flow, 25);
        uint64_t result = cs.query(flow);

        CHECK(result >= 25);
    }

    TEST_CASE("Get Parameters") {
        CountSketch cs(4, 2048);
        auto rows = cs.get_rows();
        auto cols = cs.get_cols();
        CHECK(rows == 4);
        CHECK(cols == 2048 / rows / CSSKETCH_BUCKET_SIZE);
    }

}  // TEST_SUITE