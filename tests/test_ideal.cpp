#include "../third_party/doctest.h"

#include <vector>

#include "Ideal.h"

TEST_SUITE("Ideal Tests") {
    TEST_CASE("Basic Update and Query") {
        Ideal ideal;

        TwoTuple flow(0x12345678, 0x87654321);

        ideal.update(flow, 5);
        uint64_t result = ideal.query(flow);

        CHECK(result == 5);
    }

    TEST_CASE("Multiple Updates Same Flow") {
        Ideal ideal;

        TwoTuple flow(0x11111111, 0x22222222);

        for (int i = 0; i < 10; i++) {
            ideal.update(flow);
        }

        uint64_t result = ideal.query(flow);
        CHECK(result == 10);
    }

    TEST_CASE("Insert Different Flows") {
        Ideal ideal;

        std::vector<std::pair<uint32_t, uint32_t>> flows = {
            {0xAAAAAAAA, 0xBBBBBBBB},
            {0xCCCCCCCC, 0xDDDDDDDD},
            {0xEEEEEEEE, 0xFFFFFFFF}};

        std::vector<int> counts = {15, 25, 35};

        for (size_t i = 0; i < flows.size(); i++) {
            TwoTuple flow(flows[i].first, flows[i].second);
            ideal.update(flow, counts[i]);
        }

        for (size_t i = 0; i < flows.size(); i++) {
            TwoTuple flow(flows[i].first, flows[i].second);
            uint64_t result = ideal.query(flow);
            CHECK(result == static_cast<uint64_t>(counts[i]));
        }

        CHECK(ideal.get_flow_count() == 3);
    }

    TEST_CASE("Stress Test") {
        Ideal ideal;

        TwoTuple flow(0x99999999, 0x88888888);

        uint32_t large_count = 1000000;
        ideal.update(flow, large_count);

        uint64_t result = ideal.query(flow);
        CHECK(result == large_count);
    }

    TEST_CASE("Query Non-Existent Flows") {
        Ideal ideal;

        TwoTuple flow(0x00000000, 0x00000001);
        uint64_t result = ideal.query(flow);
        CHECK(result == 0);

        bool exists = ideal.has_flow(flow);
        CHECK(!exists);
    }

    TEST_CASE("Clear") {
        Ideal ideal;

        TwoTuple flow1(0x11111111, 0x22222222);
        TwoTuple flow2(0x33333333, 0x44444444);

        ideal.update(flow1, 10);
        ideal.update(flow2, 20);

        CHECK(ideal.get_flow_count() == 2);

        ideal.clear();

        CHECK(ideal.get_flow_count() == 0);
        CHECK(ideal.query(flow1) == 0);
        CHECK(ideal.query(flow2) == 0);
    }

    TEST_CASE("Incremental Updates") {
        Ideal ideal;

        TwoTuple flow(0xABCDEF00, 0x12345678);

        ideal.update(flow, 5);
        CHECK(ideal.query(flow) == 5);

        ideal.update(flow, 3);
        CHECK(ideal.query(flow) == 8);

        ideal.update(flow, 2);
        CHECK(ideal.query(flow) == 10);
    }

    TEST_CASE("Many Different Flows") {
        Ideal ideal;

        const int num_flows = 1000;
        for (int i = 0; i < num_flows; i++) {
            TwoTuple flow(i, i * 2);
            ideal.update(flow, i);
        }

        CHECK(ideal.get_flow_count() == num_flows);

        for (int i = 0; i < num_flows; i++) {
            TwoTuple flow(i, i * 2);
            CHECK(ideal.query(flow) == static_cast<uint64_t>(i));
        }
    }

    TEST_CASE("Exact Accuracy") {
        Ideal ideal;

        TwoTuple flow1(0x12345678, 0x87654321);
        TwoTuple flow2(0x11111111, 0x22222222);
        TwoTuple flow3(0xAAAAAAAA, 0xBBBBBBBB);

        ideal.update(flow1, 100);
        ideal.update(flow2, 200);
        ideal.update(flow3, 300);

        // 精确查找
        CHECK(ideal.query(flow1) == 100);
        CHECK(ideal.query(flow2) == 200);
        CHECK(ideal.query(flow3) == 300);

        TwoTuple non_existent(0xFFFFFFFF, 0xFFFFFFFF);
        CHECK(ideal.query(non_existent) == 0);
    }

}  // TEST_SUITE
