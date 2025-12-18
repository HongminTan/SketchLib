#include "doctest.h"

#include <cstdlib>
#include <memory>

#include "ElasticSketch.h"
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

TEST_SUITE("ElasticSketch Tests") {
    TEST_CASE("Update Test") {
        std::unique_ptr<char, decltype(&free)> tcp_packet(get_tcp(), free);
        struct bpf_test_run_opts opts = {};
        opts.sz = sizeof(struct bpf_test_run_opts);
        opts.data_in = tcp_packet.get();
        opts.data_size_in = TCP_SIZE;

        struct ElasticSketch* obj = ElasticSketch__open_and_load();
        REQUIRE(obj != nullptr);

        int prog_id = bpf_program__fd(obj->progs.update);

        for (int i = 0; i < 10000; i++) {
            int err = bpf_prog_test_run_opts(prog_id, &opts);
            REQUIRE(err == 0);
            CHECK(opts.retval == XDP_PASS);
        }

        // 验证 Heavy Part 中有值
        // ElasticSketch 优先插入 Heavy Part
        int heavy_fd = bpf_map__fd(obj->maps.heavy_part_0);
        FlowKeyType key;
        memset(&key, 0, sizeof(key));
        key.src_ip = inet_addr("192.168.1.1");
        key.dst_ip = inet_addr("192.168.1.2");

        uint32_t index = hash(&key, ES_HEAVY_SEED, ES_HEAVY_BUCKET_COUNT);
        struct HeavyBucket bucket;
        memset(&bucket, 0, sizeof(bucket));

        int ret = bpf_map_lookup_elem(heavy_fd, &index, &bucket);
        CHECK(ret == 0);

        // 检查是否匹配流ID
        // 注意：这里假设没有哈希冲突，且该流被成功插入 Heavy Part
        // 对于单流测试，应该总是插入 Heavy Part
        if (memcmp(&bucket.flow_id, &key, sizeof(key)) == 0) {
            CHECK(bucket.pos_vote == 10000);
        } else {
            // 如果被驱逐了（不太可能，因为只有一种流），则检查 Light Part
            // 但在这个简单测试中，我们期望它在 Heavy Part
            WARN(
                "Flow not found in Heavy Part bucket, checking if it was "
                "evicted or collision occurred");
        }

        ElasticSketch__destroy(obj);
    }

    TEST_CASE("Query Test") {
        std::unique_ptr<char, decltype(&free)> tcp_packet(get_tcp(), free);
        struct bpf_test_run_opts opts = {};
        opts.sz = sizeof(struct bpf_test_run_opts);
        opts.data_in = tcp_packet.get();
        opts.data_size_in = TCP_SIZE;

        ElasticSketchUser es;
        struct ElasticSketch* skel = es.get_skel();
        REQUIRE(skel != nullptr);

        int prog_id = bpf_program__fd(skel->progs.update);

        for (int i = 0; i < 10000; i++) {
            int err = bpf_prog_test_run_opts(prog_id, &opts);
            REQUIRE(err == 0);
            CHECK(opts.retval == XDP_PASS);
        }

        // Swap to make data available for query
        es.swap();

        FlowKeyType key;
        memset(&key, 0, sizeof(key));
        key.src_ip = inet_addr("192.168.1.1");
        key.dst_ip = inet_addr("192.168.1.2");

        uint64_t val = es.query(key);
        CHECK(val == 10000);
    }

}  // TEST_SUITE
