#include <rte_common.h>
#include <rte_vect.h>
#include <rte_byteorder.h>
#include <rte_log.h>
#include <rte_malloc.h>
#include <rte_memory.h>
#include <rte_memcpy.h>
#include <rte_eal.h>
#include <rte_launch.h>
#include <rte_atomic.h>
#include <rte_cycles.h>
#include <rte_prefetch.h>
#include <rte_lcore.h>
#include <rte_per_lcore.h>
#include <rte_branch_prediction.h>
#include <rte_interrupts.h>
#include <rte_random.h>
#include <rte_debug.h>
#include <rte_ether.h>
#include <rte_mempool.h>
#include <rte_mbuf.h>
#include <rte_ip.h>
#include <rte_tcp.h>
#include <rte_udp.h>
#include <rte_string_fns.h>
#include <rte_cpuflags.h>
#include <stdint.h>
#include <inttypes.h>
#include <rte_ethdev.h>
#include <rte_cycles.h>
#include <rte_lcore.h>
#include <rte_mbuf.h>

#include <signal.h>
#include "FlowKey.h"
#include "CountMin.h"
#include "CountSketch.h"
#include "ElasticSketch.h"
#include "FlowRadar.h"
#include "HashPipe.h"
#include "MVSketch.h"
#include "SketchLearn.h"
#include "UnivMon.h"

#define BURST_SIZE 32

static const struct rte_eth_conf port_conf_default = {
    .rxmode = { .mq_mode = RTE_ETH_MQ_RX_NONE },
};

const int num_rx_queues = 6; // 接收队列，最多有8个，这里只设置1个接收队列数量
const int num_tx_queues = 0;

volatile bool force_quit = false;

static inline int
init_port(uint16_t port, struct rte_mempool *mbuf_pool)
{
    struct rte_eth_conf port_conf = port_conf_default;
    const uint16_t rx_rings = 1, tx_rings = 1;
    uint16_t nb_rxd = 1024;
    uint16_t nb_txd = 1024;
    int retval;
    uint16_t q;
    struct rte_eth_dev_info dev_info;
    struct rte_eth_txconf txconf;

    if (!rte_eth_dev_is_valid_port(port))
        return -1;

    retval = rte_eth_dev_info_get(port, &dev_info);
    if (retval != 0)
    {
        printf("Error during getting device (port %u) info: %s\n",
               port, strerror(-retval));
        return retval;
    }

    if (dev_info.tx_offload_capa & RTE_ETH_TX_OFFLOAD_MBUF_FAST_FREE)
        port_conf.txmode.offloads |=
            RTE_ETH_TX_OFFLOAD_MBUF_FAST_FREE;

    /* Configure the Ethernet device. */
    retval = rte_eth_dev_configure(port, rx_rings, tx_rings, &port_conf);
    if (retval != 0)
        return retval;

    retval = rte_eth_dev_adjust_nb_rx_tx_desc(port, &nb_rxd, &nb_txd);
    if (retval != 0)
        return retval;

    /* Allocate and set up 1 RX queue per Ethernet port. */
    for (q = 0; q < rx_rings; q++)
    {
        retval = rte_eth_rx_queue_setup(port, q, nb_rxd,
                                        rte_eth_dev_socket_id(port), NULL, mbuf_pool);
        if (retval < 0)
            return retval;
    }

    txconf = dev_info.default_txconf;
    txconf.offloads = port_conf.txmode.offloads;
    /* Allocate and set up 1 TX queue per Ethernet port. */
    for (q = 0; q < tx_rings; q++)
    {
        retval = rte_eth_tx_queue_setup(port, q, nb_txd,
                                        rte_eth_dev_socket_id(port), &txconf);
        if (retval < 0)
            return retval;
    }

    /* Start the Ethernet port. */
    retval = rte_eth_dev_start(port);
    if (retval < 0)
        return retval;

    /* Display the port MAC address. */
    struct rte_ether_addr addr;
    retval = rte_eth_macaddr_get(port, &addr);
    if (retval != 0)
        return retval;

    printf("Port %u MAC: %02" PRIx8 " %02" PRIx8 " %02" PRIx8
           " %02" PRIx8 " %02" PRIx8 " %02" PRIx8 "\n",
           port,
           addr.addr_bytes[0], addr.addr_bytes[1],
           addr.addr_bytes[2], addr.addr_bytes[3],
           addr.addr_bytes[4], addr.addr_bytes[5]);

    /* Enable RX in promiscuous mode for the Ethernet device. */
    retval = rte_eth_promiscuous_enable(port);
    if (retval != 0)
        return retval;

    return 0;
}

static void
signal_handler(int signum)
{
	if (signum == SIGINT || signum == SIGTERM) {
		printf("\n\nSignal %d received, preparing to exit...\n",
				signum);
		force_quit = true;
	}
}

int main(int argc, char *argv[]){
    int ret = rte_eal_init(argc, argv);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Failed to initialize EAL.\n");

    argc -= ret;
    argv += ret;

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    if (argc < 1)
        rte_exit(EXIT_FAILURE, "Invalid number of arguments.\n");

    uint16_t nb_sys_ports = rte_eth_dev_count_avail();
    if (nb_sys_ports < 1){
        rte_exit(EXIT_FAILURE, "No Supported eth found\n");
        return -1;
    }

    // 内存池初始化，发送和接收的数据都在内存池里
    struct rte_mempool *mbuf_pool = rte_pktmbuf_pool_create(
        "mbuf pool", 8192, 250, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());

    if (mbuf_pool == NULL)
        rte_exit(EXIT_FAILURE, "Could not create mbuf pool\n");
    
    // 启动dpdk
    uint16_t portid = 0;
    ret = init_port(portid, mbuf_pool);
    if (ret != 0){
        printf("port init error!\n");
    }

    printf("start to receive data!\n");

    FiveTuple ft;

    // Allocate sufficient memory to avoid division by zero (especially for SketchLearn which creates many layers)
    uint64_t memory_1mb = 1024 * 1024;
    
    CountMin<FiveTuple> cm(4, memory_1mb);
    CountSketch<FiveTuple> cs(4, memory_1mb);
    ElasticSketch<FiveTuple> es(memory_1mb / 8, 1, memory_1mb, 4);
    FlowRadar<FiveTuple> fr(memory_1mb);
    HashPipe<FiveTuple> hp(memory_1mb, 4);
    MVSketch<FiveTuple> mv(4, memory_1mb);
    SketchLearn<FiveTuple> sl(memory_1mb, 4);
    UnivMon<FiveTuple> um(4, memory_1mb);

    uint64_t total_cycles_cm = 0;
    uint64_t total_cycles_cs = 0;
    uint64_t total_cycles_es = 0;
    uint64_t total_cycles_fr = 0;
    uint64_t total_cycles_hp = 0;
    uint64_t total_cycles_mv = 0;
    uint64_t total_cycles_sl = 0;
    uint64_t total_cycles_um = 0;
    uint64_t total_pkts = 0;

    while(!force_quit){
        struct rte_mbuf *mbufs[BURST_SIZE];
        struct rte_ether_hdr *eth_hdr;

        uint16_t num_recv = rte_eth_rx_burst(portid, 0, mbufs, BURST_SIZE);
        if (num_recv > BURST_SIZE){
            // 溢出
            rte_exit(EXIT_FAILURE, "Error receiving from eth\n");
        }

        for (int i = 0; i < num_recv; i++){
            struct rte_mbuf *buf = mbufs[i];

            uint32_t sip, dip;
            uint16_t sp, dp;
            uint8_t proto_id;

            // 获取五元组

            struct rte_ipv4_hdr *ipv4_hdr = rte_pktmbuf_mtod_offset(buf, struct rte_ipv4_hdr *, sizeof(struct rte_ether_hdr));
            sip = rte_be_to_cpu_32(ipv4_hdr->src_addr);
            dip = rte_be_to_cpu_32(ipv4_hdr->dst_addr);
            proto_id = ipv4_hdr->next_proto_id;

            /* Only handle TCP packets; ensure packet length covers TCP header */
            if (proto_id != IPPROTO_TCP) {
                rte_pktmbuf_free(buf);
                continue;
            }
            if (rte_pktmbuf_pkt_len(buf) < (sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr) + sizeof(struct rte_tcp_hdr))) {
                rte_pktmbuf_free(buf);
                continue;
            }

            struct rte_tcp_hdr *tcp_hdr = rte_pktmbuf_mtod_offset(buf, struct rte_tcp_hdr *, sizeof(struct rte_ether_hdr) + sizeof(struct rte_ipv4_hdr));
            sp = rte_be_to_cpu_16(tcp_hdr->src_port);
            dp = rte_be_to_cpu_16(tcp_hdr->dst_port);

            // 给结构体赋值
            ft.src_ip = sip;
            ft.dst_ip = dip;
            ft.src_port = sp;
            ft.dst_port = dp;
            ft.protocol = proto_id;

            uint64_t start, end;

            start = rte_rdtsc();
            cm.update(ft, 1);
            end = rte_rdtsc();
            total_cycles_cm += (end - start);

            start = rte_rdtsc();
            cs.update(ft, 1);
            end = rte_rdtsc();
            total_cycles_cs += (end - start);

            start = rte_rdtsc();
            es.update(ft, 1);
            end = rte_rdtsc();
            total_cycles_es += (end - start);

            start = rte_rdtsc();
            fr.update(ft, 1);
            end = rte_rdtsc();
            total_cycles_fr += (end - start);

            start = rte_rdtsc();
            hp.update(ft, 1);
            end = rte_rdtsc();
            total_cycles_hp += (end - start);

            start = rte_rdtsc();
            mv.update(ft, 1);
            end = rte_rdtsc();
            total_cycles_mv += (end - start);

            start = rte_rdtsc();
            sl.update(ft, 1);
            end = rte_rdtsc();
            total_cycles_sl += (end - start);

            start = rte_rdtsc();
            um.update(ft, 1);
            end = rte_rdtsc();
            total_cycles_um += (end - start);

            total_pkts++;
        }

        for (uint16_t i = 0; i < num_recv; i++){
            rte_pktmbuf_free(mbufs[i]);
        }
    }

    if (total_pkts > 0) {
        printf("\nTotal packets processed: %" PRIu64 "\n", total_pkts);
        printf("Average CPU cycles per update (CountMin):     %.2f\n", (double)total_cycles_cm / total_pkts);
        printf("Average CPU cycles per update (CountSketch):  %.2f\n", (double)total_cycles_cs / total_pkts);
        printf("Average CPU cycles per update (ElasticSketch):%.2f\n", (double)total_cycles_es / total_pkts);
        printf("Average CPU cycles per update (FlowRadar):    %.2f\n", (double)total_cycles_fr / total_pkts);
        printf("Average CPU cycles per update (HashPipe):     %.2f\n", (double)total_cycles_hp / total_pkts);
        printf("Average CPU cycles per update (MVSketch):     %.2f\n", (double)total_cycles_mv / total_pkts);
        printf("Average CPU cycles per update (SketchLearn):  %.2f\n", (double)total_cycles_sl / total_pkts);
        printf("Average CPU cycles per update (UnivMon):      %.2f\n", (double)total_cycles_um / total_pkts);
    }

    rte_eth_dev_stop(portid);
    rte_eth_dev_close(portid);
    rte_mempool_free(mbuf_pool);

    // 清理 EAL 资源
    rte_eal_cleanup();
    printf("Bye...\n");

    return 0;
}