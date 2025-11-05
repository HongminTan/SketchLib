#include "FlowRadar.h"

FlowRadar::FlowRadar(uint64_t total_memory,
                     double bf_percentage,
                     uint64_t bf_num_hashes,
                     uint64_t ct_num_hashes,
                     std::unique_ptr<HashFunction> hash_function)
    : bf_num_hashes(bf_num_hashes),
      ct_num_hashes(ct_num_hashes),
      hash_function(std::move(hash_function)),
      is_decoded(false) {
    if (!this->hash_function) {
        this->hash_function = std::make_unique<DefaultHashFunction>();
    }

    // 计算 BloomFilter 和 CountingTable 的内存
    uint64_t bf_memory = total_memory * bf_percentage;
    uint64_t ct_memory = total_memory - bf_memory;

    // BloomFilter 使用位数组
    uint64_t bf_num_bits = bf_memory * 8;
    bloom_filter = std::make_unique<BloomFilter>(bf_num_bits, bf_num_hashes,
                                                 this->hash_function->clone());

    // CountingTable 的桶数
    uint64_t ct_size = ct_memory / FRBUCKET_SIZE;
    counting_table = std::vector<FRBucket>(ct_size);
}

FlowRadar::FlowRadar(const FlowRadar& other)
    : bf_num_hashes(other.bf_num_hashes),
      ct_num_hashes(other.ct_num_hashes),
      bloom_filter(std::make_unique<BloomFilter>(*other.bloom_filter)),
      counting_table(other.counting_table),
      hash_function(other.hash_function
                        ? other.hash_function->clone()
                        : std::make_unique<DefaultHashFunction>()),
      decoded_map(other.decoded_map),
      is_decoded(other.is_decoded) {}

FlowRadar& FlowRadar::operator=(const FlowRadar& other) {
    if (this != &other) {
        bf_num_hashes = other.bf_num_hashes;
        ct_num_hashes = other.ct_num_hashes;
        bloom_filter = std::make_unique<BloomFilter>(*other.bloom_filter);
        counting_table = other.counting_table;
        hash_function = other.hash_function
                            ? other.hash_function->clone()
                            : std::make_unique<DefaultHashFunction>();
        decoded_map = other.decoded_map;
        is_decoded = other.is_decoded;
    }
    return *this;
}

void FlowRadar::update(const TwoTuple& flow, int increment) {
    // FlowRadar 应为逐包处理
    for (int inc = 0; inc < increment; inc++) {
        // 查询流是否首次出现
        bool exists = (bloom_filter->query(flow) == 1);

        if (!exists) {
            // 首次出现，插入 BloomFilter
            bloom_filter->update(flow, 1);
        }

        // 组合流ID为 64 位
        uint64_t flow_key =
            (static_cast<uint64_t>(flow.src_ip) << 32) | flow.dst_ip;

        // 更新 CountingTable
        for (uint64_t i = 0; i < ct_num_hashes; i++) {
            uint64_t index =
                hash_function->hash(flow, i, counting_table.size());

            if (!exists) {
                // 首次出现：XOR 流ID，流数+1
                counting_table[index].flow_xor ^= flow_key;
                counting_table[index].flow_count++;
            }
            // 包数总是+1
            counting_table[index].packet_count++;
        }

        // 更新后需要重新解码
        is_decoded = false;
    }
}

std::map<TwoTuple, uint64_t> FlowRadar::decode() {
    // 创建 counting_table 的副本用于解码
    std::vector<FRBucket> ct_copy = counting_table;
    std::map<TwoTuple, uint64_t> result;

    // 迭代解码
    while (true) {
        bool found_pure_bucket = false;

        // 查找纯桶（flow_count == 1）
        for (size_t i = 0; i < ct_copy.size(); i++) {
            if (ct_copy[i].flow_count == 1) {
                // 找到纯桶，直接解码
                uint64_t flow_key = ct_copy[i].flow_xor;
                uint32_t packet_count = ct_copy[i].packet_count;

                // 从 64 位恢复 TwoTuple
                uint32_t src_ip = static_cast<uint32_t>(flow_key >> 32);
                uint32_t dst_ip = static_cast<uint32_t>(flow_key & 0xFFFFFFFF);
                TwoTuple flow(src_ip, dst_ip);

                // 保存结果
                result[flow] = packet_count;

                // 从所有相关桶中减去这个流
                for (uint64_t j = 0; j < ct_num_hashes; j++) {
                    uint64_t index =
                        hash_function->hash(flow, j, ct_copy.size());

                    ct_copy[index].flow_xor ^= flow_key;
                    ct_copy[index].flow_count--;
                    ct_copy[index].packet_count -= packet_count;
                }

                found_pure_bucket = true;
            }
        }

        // 没有找到纯桶，解码结束
        if (!found_pure_bucket) {
            break;
        }
    }

    // 缓存解码结果
    decoded_map = result;
    is_decoded = true;

    return result;
}

uint64_t FlowRadar::query(const TwoTuple& flow) {
    // 如果还没解码，先解码
    if (!is_decoded) {
        decode();
    }

    // 从解码结果中查找
    auto it = decoded_map.find(flow);
    if (it != decoded_map.end()) {
        return it->second;
    }

    // 找不到返回 0
    return 0;
}
