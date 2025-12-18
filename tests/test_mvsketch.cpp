#include "doctest.h"

#include <cstdlib>
#include <memory>

#include "FlowKey.h"
#include "MVSketch.h"
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

TEST_SUITE("MVSketch Tests") {
    TEST_CASE("Update Test") {
        std::unique_ptr<char, decltype(&free)> tcp_packet(get_tcp(), free);
        struct bpf_test_run_opts opts = {};
        opts.sz = sizeof(struct bpf_test_run_opts);
        opts.data_in = tcp_packet.get();
        opts.data_size_in = TCP_SIZE;

        struct MVSketch* obj = MVSketch__open_and_load();
        REQUIRE(obj != nullptr);

        int prog_id = bpf_program__fd(obj->progs.update);

        for (int i = 0; i < 10000; i++) {
            int err = bpf_prog_test_run_opts(prog_id, &opts);
            REQUIRE(err == 0);
            CHECK(opts.retval == XDP_PASS);
        }

        // 验证 map 中有值
        int map_fd = bpf_map__fd(obj->maps.buckets_0);
        FlowKeyType key;
        memset(&key, 0, sizeof(key));
        key.src_ip = inet_addr("192.168.1.1");
        key.dst_ip = inet_addr("192.168.1.2");

        for (int row = 0; row < MV_ROWS; ++row) {
            uint32_t index = hash(&key, row, MV_COLS);
            uint32_t offset = (uint32_t)(row * MV_COLS + index);
            struct MVBucket bucket;
            memset(&bucket, 0, sizeof(bucket));

            int ret = bpf_map_lookup_elem(map_fd, &offset, &bucket);
            CHECK(ret == 0);

            // 检查总计数
            CHECK(bucket.value == 10000);

            // 检查流ID和特定计数
            if (memcmp(&bucket.flow_id, &key, sizeof(key)) == 0) {
                CHECK(bucket.count == 10000);
            }
        }

        MVSketch__destroy(obj);
    }

    TEST_CASE("Query Test") {
        std::unique_ptr<char, decltype(&free)> tcp_packet(get_tcp(), free);
        struct bpf_test_run_opts opts = {};
        opts.sz = sizeof(struct bpf_test_run_opts);
        opts.data_in = tcp_packet.get();
        opts.data_size_in = TCP_SIZE;

        MVSketchUser mv;
        struct MVSketch* skel = mv.get_skel();
        REQUIRE(skel != nullptr);

        int prog_id = bpf_program__fd(skel->progs.update);

        for (int i = 0; i < 10000; i++) {
            int err = bpf_prog_test_run_opts(prog_id, &opts);
            REQUIRE(err == 0);
            CHECK(opts.retval == XDP_PASS);
        }

        // Swap to make data available for query
        mv.swap();

        FlowKeyType key;
        memset(&key, 0, sizeof(key));
        key.src_ip = inet_addr("192.168.1.1");
        key.dst_ip = inet_addr("192.168.1.2");

        uint64_t val = mv.query(key);
        CHECK(val == 10000);
    }

}  // TEST_SUITE
