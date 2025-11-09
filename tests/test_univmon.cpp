#include <cstdint>
#include <memory>
#include <vector>
#include "../third_party/doctest.h"

#include "FlowKey.h"
#include "HashFunction.h"
#include "UnivMon.h"

TEST_SUITE("UnivMon Tests") {
    TEST_CASE("CountSketch Backend") {
        UnivMon<TwoTuple> um(4, 4096);

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
        UnivMon<TwoTuple> um(3, 2048, nullptr, UnivMonBackend::SaH);

        TwoTuple flow(0xAABBCCDD, 0x11223344);

        um.update(flow, 50);
        uint64_t result = um.query(flow);

        // 小规模准确率不高
        CHECK(result > 0);
        CHECK(um.get_backend() == UnivMonBackend::SaH);
    }

    TEST_CASE("Heavy Hitter") {
        UnivMon<TwoTuple> um(6, 16384, nullptr, UnivMonBackend::CountSketch);

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
        UnivMon<TwoTuple> um(4, 4096);

        TwoTuple flow(0x12345678, 0x87654321);

        um.update(flow, 0);
        uint64_t result = um.query(flow);

        CHECK(result == 0);
    }

    TEST_CASE("OneTuple Support") {
        UnivMon<OneTuple> um(3, 2048);
        OneTuple flow(0xFEDCBA98);

        for (int i = 0; i < 1000; i++) {
            um.update(flow);
        }
        uint64_t result = um.query(flow);

        CHECK(result >= 900);
        CHECK(result <= 1100);
    }

    TEST_CASE("FiveTuple Support") {
        UnivMon<FiveTuple> um(3, 2048);
        FiveTuple flow(0x12345678, 0x87654321, 80, 443, 6);

        for (int i = 0; i < 1000; i++) {
            um.update(flow);
        }
        uint64_t result = um.query(flow);

        CHECK(result >= 900);
        CHECK(result <= 1100);
    }

}  // TEST_SUITE
