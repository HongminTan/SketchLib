#ifndef TWOTUPLE_H
#define TWOTUPLE_H

#include <cstdint>
#include <functional>

/**
 * @brief 二元组类 (src_ip, dst_ip)
 */
class TwoTuple {
   public:
    uint32_t src_ip;
    uint32_t dst_ip;

    TwoTuple() : src_ip(0), dst_ip(0) {}

    TwoTuple(uint32_t src, uint32_t dst) : src_ip(src), dst_ip(dst) {}

    bool operator==(const TwoTuple& other) const {
        return src_ip == other.src_ip && dst_ip == other.dst_ip;
    }

    bool operator!=(const TwoTuple& other) const { return !(*this == other); }

    /**
     * @brief 小于比较，用于有序容器
     */
    bool operator<(const TwoTuple& other) const {
        if (src_ip != other.src_ip) {
            return src_ip < other.src_ip;
        }
        return dst_ip < other.dst_ip;
    }
};

/**
 * @brief TwoTuple 的哈希函数对象，用于支持 std::unordered_map
 *
 * 使用组合的 64 位哈希值来保证库内的一致性哈希
 */
struct TwoTupleHash {
    std::size_t operator()(const TwoTuple& tuple) const {
        uint64_t combined = (static_cast<uint64_t>(tuple.src_ip) << 32) ^
                            static_cast<uint64_t>(tuple.dst_ip);
        return static_cast<std::size_t>(std::hash<uint64_t>{}(combined));
    }
};

#endif /* TWOTUPLE_H */
