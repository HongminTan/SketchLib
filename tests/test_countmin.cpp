#include "../third_party/doctest.h"

#include "CountMin.h"
#include "HashFunction.h"

TEST_SUITE("CountMin Tests") {
    TEST_CASE("Default Hash Function") {
        CountMin cm(4, 1024);

        TwoTuple flow(0x12345678, 0x87654321);

        cm.update(flow, 5);
        uint64_t result = cm.query(flow);

        CHECK(result >= 5);
    }

    TEST_CASE("Custom Hash Function - SpookyV2") {
        auto custom_hash = std::make_unique<SpookyV2HashFunction>();
        CountMin cm(5, 2048, std::move(custom_hash));

        TwoTuple flow(0x11111111, 0x22222222);

        for (int i = 0; i < 10; i++) {
            cm.update(flow);
        }

        uint64_t result = cm.query(flow);
        CHECK(result >= 10);
    }

    TEST_CASE("Custom Hash Function - MurmurV3") {
        auto custom_hash = std::make_unique<MurmurV3HashFunction>();
        CountMin cm(5, 2048, std::move(custom_hash));

        TwoTuple flow(0xAABBCCDD, 0xEEFF0011);

        for (int i = 0; i < 15; i++) {
            cm.update(flow);
        }

        uint64_t result = cm.query(flow);
        CHECK(result >= 15);
    }

    TEST_CASE("Insert Different Flows") {
        CountMin cm(6, 4096);

        std::vector<std::pair<uint32_t, uint32_t>> flows = {
            {0xAAAAAAAA, 0xBBBBBBBB},
            {0xCCCCCCCC, 0xDDDDDDDD},
            {0xEEEEEEEE, 0xFFFFFFFF}};

        std::vector<int> counts = {15, 25, 35};

        for (size_t i = 0; i < flows.size(); i++) {
            TwoTuple flow(flows[i].first, flows[i].second);
            cm.update(flow, counts[i]);
        }

        for (size_t i = 0; i < flows.size(); i++) {
            TwoTuple flow(flows[i].first, flows[i].second);
            uint64_t result = cm.query(flow);
            CHECK(result >= static_cast<uint64_t>(counts[i]));
        }
    }

    TEST_CASE("Stress Test") {
        CountMin cm(3, 1024);

        TwoTuple flow(0x99999999, 0x88888888);

        uint32_t large_count = 1000000;
        cm.update(flow, large_count);

        uint64_t result = cm.query(flow);
        CHECK(result >= large_count);
    }

    TEST_CASE("Query Non-Existent Flows") {
        CountMin cm(4, 1024);

        TwoTuple flow(0x00000000, 0x00000001);
        uint64_t result = cm.query(flow);
        CHECK(result == 0);

        bool exists = cm.has_flow(flow);
        CHECK(!exists);
    }

    TEST_CASE("Get Parameters") {
        CountMin cm(7, 2800);
        auto rows = cm.get_rows();
        auto cols = cm.get_cols();
        CHECK(rows == 7);
        CHECK(cols == 2800 / rows / CMBUCKET_SIZE);
    }

}  // TEST_SUITE