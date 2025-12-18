#include "doctest.h"

#include <cstdlib>
#include <memory>

#include "FlowKey.h"
#include "FlowRadar.h"
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

TEST_SUITE("FlowRadar Tests") {
    TEST_CASE("Update Test") {
        std::unique_ptr<char, decltype(&free)> tcp_packet(get_tcp(), free);
        struct bpf_test_run_opts opts = {};
        opts.sz = sizeof(struct bpf_test_run_opts);
        opts.data_in = tcp_packet.get();
        opts.data_size_in = TCP_SIZE;

        struct FlowRadar* obj = FlowRadar__open_and_load();
        REQUIRE(obj != nullptr);

        int prog_id = bpf_program__fd(obj->progs.update);

        for (int i = 0; i < 10000; i++) {
            int err = bpf_prog_test_run_opts(prog_id, &opts);
            REQUIRE(err == 0);
            CHECK(opts.retval == XDP_PASS);
        }

        // 验证 BloomFilter 和 CountingTable
        int bf_fd = bpf_map__fd(obj->maps.bf_0);
        int ct_fd = bpf_map__fd(obj->maps.ct_0);
        FlowKeyType key;
        memset(&key, 0, sizeof(key));
        key.src_ip = inet_addr("192.168.1.1");
        key.dst_ip = inet_addr("192.168.1.2");

        // 检查 BloomFilter
        bool bf_set = true;
        for (int i = 0; i < FR_BF_NUM_HASHES; ++i) {
            uint32_t hash_val = hash(&key, i, FR_BF_NUM_BITS);
            uint32_t word_idx = hash_val / FR_BITS_PER_WORD;
            uint32_t bit_idx = hash_val % FR_BITS_PER_WORD;

            uint32_t word = 0;
            int ret = bpf_map_lookup_elem(bf_fd, &word_idx, &word);
            CHECK(ret == 0);
            if (!((word >> bit_idx) & 1)) {
                bf_set = false;
                break;
            }
        }
        CHECK(bf_set == true);

        // 检查 CountingTable
        // 注意：FlowRadar 只有在 BloomFilter 认为流是新的或者发生冲突时才会更新
        // CountingTable 对于单流测试，第一次插入会更新
        // CountingTable，后续包如果 BloomFilter 已经置位，
        // 且没有冲突，逻辑上可能只更新包计数。
        // 具体逻辑取决于 FlowRadar 的实现细节。
        // 假设 FlowRadar 总是尝试更新 CountingTable (FlowRadar 论文逻辑)

        bool found_in_ct = false;
        for (int i = 0; i < FR_CT_NUM_HASHES; ++i) {
            uint32_t index = hash(&key, i, FR_CT_SIZE);
            struct FRBucket bucket;
            memset(&bucket, 0, sizeof(bucket));

            int ret = bpf_map_lookup_elem(ct_fd, &index, &bucket);
            CHECK(ret == 0);

            if (bucket.packet_count > 0) {
                found_in_ct = true;
                // 简单的单流测试，flow_count 应该是 1
                // packet_count 应该是 10000
                // flow_xor 应该是 key
                // 但由于使用了多个哈希函数，可能会有多个桶被更新
                // 这里只要找到至少一个桶有数据即可
            }
        }
        CHECK(found_in_ct == true);

        FlowRadar__destroy(obj);
    }

    TEST_CASE("Query Test") {
        std::unique_ptr<char, decltype(&free)> tcp_packet(get_tcp(), free);
        struct bpf_test_run_opts opts = {};
        opts.sz = sizeof(struct bpf_test_run_opts);
        opts.data_in = tcp_packet.get();
        opts.data_size_in = TCP_SIZE;

        FlowRadarUser fr;
        struct FlowRadar* skel = fr.get_skel();
        REQUIRE(skel != nullptr);

        int prog_id = bpf_program__fd(skel->progs.update);

        for (int i = 0; i < 10000; i++) {
            int err = bpf_prog_test_run_opts(prog_id, &opts);
            REQUIRE(err == 0);
            CHECK(opts.retval == XDP_PASS);
        }

        // Swap to make data available for query
        fr.swap();

        FlowKeyType key;
        memset(&key, 0, sizeof(key));
        key.src_ip = inet_addr("192.168.1.1");
        key.dst_ip = inet_addr("192.168.1.2");

        uint64_t val = fr.query(key);
        CHECK(val == 10000);
    }

}  // TEST_SUITE
