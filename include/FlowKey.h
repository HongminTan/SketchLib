#ifndef FLOWKEY_H
#define FLOWKEY_H

#include <cstdint>
#include <functional>
#include <type_traits>

/**
 * @brief 一元组
 * 用于单字段流标识，如单个IP地址
 */
struct OneTuple {
    uint32_t ip;

    OneTuple() : ip(0) {}
    explicit OneTuple(uint32_t ip_val) : ip(ip_val) {}

    bool operator==(const OneTuple& other) const { return ip == other.ip; }

    bool operator!=(const OneTuple& other) const { return !(*this == other); }

    bool operator<(const OneTuple& other) const { return ip < other.ip; }

    OneTuple& operator^=(const OneTuple& other) {
        ip ^= other.ip;
        return *this;
    }

    OneTuple operator^(const OneTuple& other) const {
        OneTuple result = *this;
        result ^= other;
        return result;
    }
};

/**
 * @brief 二元组
 * 用于双字段流标识，如源IP和目标IP
 */
struct TwoTuple {
    uint32_t src_ip;
    uint32_t dst_ip;

    TwoTuple() : src_ip(0), dst_ip(0) {}
    TwoTuple(uint32_t src, uint32_t dst) : src_ip(src), dst_ip(dst) {}

    bool operator==(const TwoTuple& other) const {
        return src_ip == other.src_ip && dst_ip == other.dst_ip;
    }

    bool operator!=(const TwoTuple& other) const { return !(*this == other); }

    bool operator<(const TwoTuple& other) const {
        if (src_ip != other.src_ip) {
            return src_ip < other.src_ip;
        }
        return dst_ip < other.dst_ip;
    }

    TwoTuple& operator^=(const TwoTuple& other) {
        src_ip ^= other.src_ip;
        dst_ip ^= other.dst_ip;
        return *this;
    }

    TwoTuple operator^(const TwoTuple& other) const {
        TwoTuple result = *this;
        result ^= other;
        return result;
    }
};

/**
 * @brief 五元组
 * 用于完整的五元组流标识
 */
struct FiveTuple {
    uint32_t src_ip;
    uint32_t dst_ip;
    uint16_t src_port;
    uint16_t dst_port;
    uint8_t protocol;
    uint8_t padding[3];

    FiveTuple()
        : src_ip(0),
          dst_ip(0),
          src_port(0),
          dst_port(0),
          protocol(0),
          padding{0} {}

    FiveTuple(uint32_t src_ip_val,
              uint32_t dst_ip_val,
              uint16_t src_port_val,
              uint16_t dst_port_val,
              uint8_t protocol_val)
        : src_ip(src_ip_val),
          dst_ip(dst_ip_val),
          src_port(src_port_val),
          dst_port(dst_port_val),
          protocol(protocol_val),
          padding{0} {}

    bool operator==(const FiveTuple& other) const {
        return src_ip == other.src_ip && dst_ip == other.dst_ip &&
               src_port == other.src_port && dst_port == other.dst_port &&
               protocol == other.protocol;
    }

    bool operator!=(const FiveTuple& other) const { return !(*this == other); }

    bool operator<(const FiveTuple& other) const {
        if (src_ip != other.src_ip)
            return src_ip < other.src_ip;
        if (dst_ip != other.dst_ip)
            return dst_ip < other.dst_ip;
        if (src_port != other.src_port)
            return src_port < other.src_port;
        if (dst_port != other.dst_port)
            return dst_port < other.dst_port;
        return protocol < other.protocol;
    }

    FiveTuple& operator^=(const FiveTuple& other) {
        src_ip ^= other.src_ip;
        dst_ip ^= other.dst_ip;
        src_port ^= other.src_port;
        dst_port ^= other.dst_port;
        protocol ^= other.protocol;
        return *this;
    }

    FiveTuple operator^(const FiveTuple& other) const {
        FiveTuple result = *this;
        result ^= other;
        return result;
    }
};

/**
 * @brief 类型特征：判断类型是否为有效的 FlowKey
 */
template <typename T>
struct is_flowkey : std::false_type {};

template <>
struct is_flowkey<OneTuple> : std::true_type {};

template <>
struct is_flowkey<TwoTuple> : std::true_type {};

template <>
struct is_flowkey<FiveTuple> : std::true_type {};

/**
 * @brief SFINAE 模板类型约束
 * 只有当 T 是有效的 FlowKey 类型时，RequireFlowKey 才会被成功推导
 */
template <typename T>
using RequireFlowKey = typename std::enable_if<is_flowkey<T>::value>::type;

// 为 std::hash 提供特化
namespace std {
template <>
struct hash<OneTuple> {
    std::size_t operator()(const OneTuple& key) const {
        return std::hash<uint32_t>{}(key.ip);
    }
};

template <>
struct hash<TwoTuple> {
    std::size_t operator()(const TwoTuple& key) const {
        uint64_t combined =
            (static_cast<uint64_t>(key.src_ip) << 32) | key.dst_ip;
        return std::hash<uint64_t>{}(combined);
    }
};

template <>
struct hash<FiveTuple> {
    std::size_t operator()(const FiveTuple& key) const {
        std::size_t h1 = std::hash<uint32_t>{}(key.src_ip);
        std::size_t h2 = std::hash<uint32_t>{}(key.dst_ip);
        std::size_t h3 = std::hash<uint16_t>{}(key.src_port);
        std::size_t h4 = std::hash<uint16_t>{}(key.dst_port);
        std::size_t h5 = std::hash<uint8_t>{}(key.protocol);
        return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3) ^ (h5 << 4);
    }
};
}  // namespace std

#endif /* FLOWKEY_H */
