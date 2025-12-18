#include "doctest.h"

#include <cstdlib>
#include <memory>

#include "CountSketch.h"
#include "FlowKey.h"
#include "hash.h"

#include <arpa/inet.h>
#include <linux/icmp.h>
#include <linux/if_ether.h>
#include <linux/if_xdp.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <cstring>

#define TCP_SIZE 128

static char* get_tcp() {
    char* buf = (char*)malloc(TCP_SIZE);
    if (!buf)
        return nullptr;
    memset(buf, 0, TCP_SIZE);

    struct ethhdr* eth = (struct ethhdr*)buf;
    struct iphdr* ip = (struct iphdr*)(buf + sizeof(struct ethhdr));
    struct tcphdr* tcp =
        (struct tcphdr*)(buf + sizeof(struct ethhdr) + sizeof(struct iphdr));

    // Ethernet
    eth->h_proto = htons(ETH_P_IP);

    // IP
    ip->ihl = 5;
    ip->version = 4;
    ip->protocol = IPPROTO_TCP;
    ip->saddr = inet_addr("192.168.1.1");
    ip->daddr = inet_addr("192.168.1.2");
    ip->tot_len = htons(TCP_SIZE - sizeof(struct ethhdr));
    ip->ttl = 64;

    // TCP
    tcp->source = htons(1234);
    tcp->dest = htons(80);
    tcp->doff = 5;

    return buf;
}

TEST_SUITE("CountSketch Tests") {
    TEST_CASE("Update Test") {
        std::unique_ptr<char, decltype(&free)> tcp_packet(get_tcp(), free);
        struct bpf_test_run_opts opts = {};
        opts.sz = sizeof(struct bpf_test_run_opts);
        opts.data_in = tcp_packet.get();
        opts.data_size_in = TCP_SIZE;

        struct CountSketch* obj = CountSketch__open_and_load();
        REQUIRE(obj != nullptr);

        int prog_id = bpf_program__fd(obj->progs.update);

        for (int i = 0; i < 10000; i++) {
            int err = bpf_prog_test_run_opts(prog_id, &opts);
            REQUIRE(err == 0);
            CHECK(opts.retval == XDP_PASS);
        }

        // 验证 map 中有值
        int map_fd = bpf_map__fd(obj->maps.counters_0);
        FlowKeyType key;
        memset(&key, 0, sizeof(key));
        key.src_ip = inet_addr("192.168.1.1");
        key.dst_ip = inet_addr("192.168.1.2");

        // CountSketch 的计数可能为正也可能为负，取决于 hash 符号
        // 这里我们只验证是否有非零值，或者验证绝对值是否合理（如果哈希冲突不严重）
        // 由于只发了一种流，所有行应该都有值
        for (int row = 0; row < CS_ROWS; ++row) {
            uint32_t index = hash(&key, row, CS_COLS);
            uint32_t offset = (uint32_t)(row * CS_COLS + index);
            CS_COUNTER_TYPE val = 0;
            int ret = bpf_map_lookup_elem(map_fd, &offset, &val);
            CHECK(ret == 0);
            CHECK(val != 0);
            CHECK(abs(val) == 10000);
        }

        CountSketch__destroy(obj);
    }

    TEST_CASE("Query Test") {
        std::unique_ptr<char, decltype(&free)> tcp_packet(get_tcp(), free);
        struct bpf_test_run_opts opts = {};
        opts.sz = sizeof(struct bpf_test_run_opts);
        opts.data_in = tcp_packet.get();
        opts.data_size_in = TCP_SIZE;

        CountSketchUser cs;
        struct CountSketch* skel = cs.get_skel();
        REQUIRE(skel != nullptr);

        int prog_id = bpf_program__fd(skel->progs.update);

        for (int i = 0; i < 10000; i++) {
            int err = bpf_prog_test_run_opts(prog_id, &opts);
            REQUIRE(err == 0);
            CHECK(opts.retval == XDP_PASS);
        }

        // Swap to make data available for query
        cs.swap();

        FlowKeyType key;
        memset(&key, 0, sizeof(key));
        key.src_ip = inet_addr("192.168.1.1");
        key.dst_ip = inet_addr("192.168.1.2");

        uint64_t val = cs.query(key);
        CHECK(val == 10000);
    }

}  // TEST_SUITE
