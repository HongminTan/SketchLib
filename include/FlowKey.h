#ifndef FLOWKEY_H
#define FLOWKEY_H

#ifdef __BPF__
#include "autogen/vmlinux.h"

#include <bpf/bpf_endian.h>
#include <bpf/bpf_helpers.h>
#else
#include <stdint.h>
#endif /* __BPF__ */

struct OneTuple {
    uint32_t ip;
};

struct TwoTuple {
    uint32_t src_ip;
    uint32_t dst_ip;
};

struct FiveTuple {
    uint32_t src_ip;
    uint32_t dst_ip;
    uint16_t src_port;
    uint16_t dst_port;
    uint8_t protocol;
    uint8_t padding[3];
};

_Static_assert(sizeof(struct OneTuple) == 4, "OneTuple size must be 4 bytes");
_Static_assert(sizeof(struct TwoTuple) == 8, "TwoTuple size must be 8 bytes");
_Static_assert(sizeof(struct FiveTuple) == 16,
               "FiveTuple size must be 16 bytes");

// 定义 FlowKeyType

// #define OneTupleType 1
#define TwoTupleType 2
// #define FiveTupleType 5

#ifdef OneTupleType
typedef struct OneTuple FlowKey;
#define flowkey_extractor extract_one_tuple
#elif defined(TwoTupleType)
typedef struct TwoTuple FlowKeyType;
#define flowkey_extractor extract_two_tuple
#elif defined(FiveTupleType)
typedef struct FiveTuple FlowKey;
#define flowkey_extractor extract_five_tuple
#endif

#ifdef __BPF__
// 通用的 FlowKey 解析接口
#define extract_flowkey(ctx, key) flowkey_extractor((ctx), (key))

// 以下是 FlowKey 解析函数

#ifndef ETH_P_IP
#define ETH_P_IP 0x0800
#endif

#ifndef ETH_P_8021Q
#define ETH_P_8021Q 0x8100
#endif

#ifndef ETH_P_8021AD
#define ETH_P_8021AD 0x88A8
#endif

/*
 * @brief 解析 IPv4 头部及其后的传输层
 * @param ctx XDP 上下文
 * @param iph 输出参数，指向解析出的 IPv4 头部
 * @param l4hdr 输出参数，指向传输层头部(协议未知)
 * @return 0 成功，-1 失败
 */
static __always_inline int parse_ipv4_headers(struct xdp_md* ctx,
                                              struct iphdr** iph,
                                              void** l4hdr) {
    void* data = (void*)(long)ctx->data;
    void* data_end = (void*)(long)ctx->data_end;

    struct ethhdr* eth = data;
    if ((void*)(eth + 1) > data_end)
        return -1;

    uint16_t proto = bpf_ntohs(eth->h_proto);
    void* nh = eth + 1;

    // 处理两层 vlans
#pragma unroll
    for (int i = 0; i < 2; ++i) {
        if (proto == ETH_P_8021Q || proto == ETH_P_8021AD) {
            struct vlan_hdr* vh = nh;
            if ((void*)(vh + 1) > data_end)
                return -1;
            proto = bpf_ntohs(vh->h_vlan_encapsulated_proto);
            nh = vh + 1;
        }
    }

    // 非 IPv4 包
    if (proto != ETH_P_IP)
        return -1;

    // 解析 IPv4 头
    struct iphdr* ip = nh;
    if ((void*)(ip + 1) > data_end)
        return -1;
    uint8_t ihl = ip->ihl;
    if (ihl < 5)
        return -1;
    uint64_t ip_header_len = (uint64_t)ihl << 2;
    if ((uint8_t*)ip + ip_header_len > (uint8_t*)data_end)
        return -1;
    if (iph)
        *iph = ip;

    // 解析传输层头
    if (l4hdr)
        *l4hdr = (uint8_t*)ip + ip_header_len;

    return 0;
}

static __always_inline int extract_one_tuple(struct xdp_md* ctx,
                                             struct OneTuple* key) {
    if (!key)
        return -1;

    __builtin_memset(key, 0, sizeof(*key));

    struct iphdr* ip = NULL;
    if (parse_ipv4_headers(ctx, &ip, NULL))
        return -1;

    key->ip = ip->saddr;
    return 0;
}

static __always_inline int extract_two_tuple(struct xdp_md* ctx,
                                             struct TwoTuple* key) {
    if (!key)
        return -1;

    __builtin_memset(key, 0, sizeof(*key));

    struct iphdr* ip = NULL;
    if (parse_ipv4_headers(ctx, &ip, NULL))
        return -1;

    key->src_ip = ip->saddr;
    key->dst_ip = ip->daddr;

    return 0;
}

static __always_inline int extract_five_tuple(struct xdp_md* ctx,
                                              struct FiveTuple* key) {
    if (!key)
        return -1;

    __builtin_memset(key, 0, sizeof(*key));

    struct iphdr* ip = NULL;
    void* l4 = NULL;
    if (parse_ipv4_headers(ctx, &ip, &l4))
        return -1;

    key->src_ip = ip->saddr;
    key->dst_ip = ip->daddr;
    key->protocol = ip->protocol;

    void* data_end = (void*)(long)ctx->data_end;

    if (ip->protocol == IPPROTO_TCP) {
        struct tcphdr* tcp = l4;
        if ((void*)(tcp + 1) > data_end)
            return -1;
        key->src_port = tcp->source;
        key->dst_port = tcp->dest;
    } else if (ip->protocol == IPPROTO_UDP) {
        struct udphdr* udp = l4;
        if ((void*)(udp + 1) > data_end)
            return -1;
        key->src_port = udp->source;
        key->dst_port = udp->dest;
    } else {
        key->src_port = 0;
        key->dst_port = 0;
    }

    return 0;
}
#endif /* __BPF__ */

#endif /* FLOWKEY_H */