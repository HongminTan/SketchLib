#include <cstdint>
#include <memory>
#include <vector>
#include "../third_party/doctest.h"

#include "HashFunction.h"
#include "UnivMon.h"

TEST_SUITE("UnivMon Tests") {
    TEST_CASE("CountSketch Backend") {
        UnivMon um(4, 4096);

        TwoTuple flow(0x12345678, 0x87654321);

        um.update(flow, 100);
        uint64_t result = um.query(flow);

        // 小规模准确率不高
        CHECK(result > 0);
        CHECK(um.get_layer_count() == 4);
        CHECK(um.get_memory_budget() == 4096);
        CHECK(um.get_backend() == UnivMonBackend::CountSketch);
    }

    TEST_CASE("SaH Backend") {
        UnivMon um(3, 2048, nullptr, UnivMonBackend::SaH);

        TwoTuple flow(0xAABBCCDD, 0x11223344);

        um.update(flow, 50);
        uint64_t result = um.query(flow);

        // 小规模准确率不高
        CHECK(result > 0);
        CHECK(um.get_backend() == UnivMonBackend::SaH);
    }

    TEST_CASE("Heavy Hitter") {
        UnivMon um(6, 16384, nullptr, UnivMonBackend::CountSketch);

        TwoTuple heavyFlow(0x11111111, 0x22222222);

        for (int i = 0; i < 1000; i++) {
            um.update(heavyFlow);
        }

        uint64_t heavyResult = um.query(heavyFlow);
        // 大规模允许一定误差
        CHECK(heavyResult >= 900);
        CHECK(heavyResult <= 1100);
    }

    TEST_CASE("Zero Update") {
        UnivMon um(4, 4096);

        TwoTuple flow(0x12345678, 0x87654321);

        um.update(flow, 0);
        uint64_t result = um.query(flow);

        CHECK(result == 0);
    }

}  // TEST_SUITE
