#!/bin/bash

# XDP 测试网络环境管理脚本
# 仅用于测试 XDP 程序正确性
# 
# 网络拓扑:
# ┌─────────────────────┐     veth pair     ┌─────────────────────┐
# │   sender 命名空间    │◄────────────────►│    主命名空间        │
# │                     │                   │                     │
# │   veth-send         │                   │   veth-recv         │
# │   10.10.10.1/24     │                   │   10.10.10.2/24     │
# │                     │                   │   ( XDP 挂载)       │
# └─────────────────────┘                   └─────────────────────┘
#
# 使用方法:
#   sudo ./network.sh setup    - 创建网络环境
#   sudo ./network.sh cleanup  - 清理网络环境
#
# 测试:
#   1. 在主命名空间运行 XDP 程序: sudo ./xdp_program veth-recv
#   2. 在 sender 命名空间发送包:  sudo ip netns exec sender send_program

set -e

NETNS_NAME="sender"
VETH_SEND="veth-send"
VETH_RECV="veth-recv"
IP_SEND="10.10.10.1/24"
IP_RECV="10.10.10.2/24"

cleanup() {
    # 删除网络命名空间（会自动删除整个 veth pair）
    ip netns del ${NETNS_NAME} 2>/dev/null || true
    echo "cleanup done"
}

setup() {
    # 清理旧配置
    cleanup

    # 创建网络命名空间
    ip netns add ${NETNS_NAME}

    # 创建 veth pair
    ip link add ${VETH_SEND} type veth peer name ${VETH_RECV}

    # 将 veth-send 移到 sender 命名空间
    ip link set ${VETH_SEND} netns ${NETNS_NAME}

    # 配置主命名空间的 veth-recv
    ip addr add ${IP_RECV} dev ${VETH_RECV}
    ip link set ${VETH_RECV} up

    # 配置 sender 命名空间的 veth-send
    ip netns exec ${NETNS_NAME} ip addr add ${IP_SEND} dev ${VETH_SEND}
    ip netns exec ${NETNS_NAME} ip link set ${VETH_SEND} up
    ip netns exec ${NETNS_NAME} ip link set lo up

    echo "setup done"
}

case "${1:-}" in
    setup)   setup ;;
    cleanup) cleanup ;;
    *)
        echo "Usage: $0 {setup|cleanup}"
        exit 1
        ;;
esac
