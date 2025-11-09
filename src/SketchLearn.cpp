#include "SketchLearn.h"

template <typename FlowKeyType, typename SFINAE>
SketchLearn<FlowKeyType, SFINAE>::SketchLearn(
    uint64_t total_memory,
    uint64_t num_rows,
    double theta,
    std::unique_ptr<HashFunction<FlowKeyType>> hash_function)
    : num_rows(num_rows),
      num_cols(0),
      hash_function(std::move(hash_function)),
      theta(theta),
      p(FLOWKEY_BITS + 1, 0.0),
      variance(FLOWKEY_BITS + 1, 0.0),
      is_decoded(false) {
    if (!this->hash_function) {
        this->hash_function =
            std::make_unique<DefaultHashFunction<FlowKeyType>>();
    }

    // 计算每层的内存
    uint64_t layer_memory = total_memory / (FLOWKEY_BITS + 1);

    // 创建 l+1 层 CountMin
    for (size_t i = 0; i <= FLOWKEY_BITS; i++) {
        layers.push_back(std::make_unique<CountMin<FlowKeyType>>(
            num_rows, layer_memory, this->hash_function->clone()));
    }

    num_cols = layers[0]->get_cols();
}

template <typename FlowKeyType, typename SFINAE>
SketchLearn<FlowKeyType, SFINAE>::SketchLearn(const SketchLearn& other)
    : num_rows(other.num_rows),
      num_cols(other.num_cols),
      hash_function(other.hash_function
                        ? other.hash_function->clone()
                        : std::make_unique<DefaultHashFunction<FlowKeyType>>()),
      theta(other.theta),
      p(other.p),
      variance(other.variance),
      decoded_map(other.decoded_map),
      is_decoded(other.is_decoded) {
    for (const auto& layer : other.layers) {
        layers.push_back(std::make_unique<CountMin<FlowKeyType>>(*layer));
    }
}

template <typename FlowKeyType, typename SFINAE>
SketchLearn<FlowKeyType, SFINAE>& SketchLearn<FlowKeyType, SFINAE>::operator=(
    const SketchLearn& other) {
    if (this != &other) {
        num_rows = other.num_rows;
        num_cols = other.num_cols;
        hash_function =
            other.hash_function
                ? other.hash_function->clone()
                : std::make_unique<DefaultHashFunction<FlowKeyType>>();
        theta = other.theta;
        p = other.p;
        variance = other.variance;
        decoded_map = other.decoded_map;
        is_decoded = other.is_decoded;

        layers.clear();
        for (const auto& layer : other.layers) {
            layers.push_back(std::make_unique<CountMin<FlowKeyType>>(*layer));
        }
    }
    return *this;
}

// 通用 flow_to_bits
template <typename FlowKeyType, typename SFINAE>
std::bitset<SketchLearn<FlowKeyType, SFINAE>::FLOWKEY_BITS>
SketchLearn<FlowKeyType, SFINAE>::flow_to_bits(const FlowKeyType& flow) const {
    std::bitset<FLOWKEY_BITS> bits;
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&flow);

    for (size_t i = 0; i < FLOWKEY_SIZE; ++i) {
        for (size_t j = 0; j < 8; ++j) {
            bits[i * 8 + j] = (bytes[i] >> j) & 1;
        }
    }

    return bits;
}

// 通用 bits_to_flow
template <typename FlowKeyType, typename SFINAE>
FlowKeyType SketchLearn<FlowKeyType, SFINAE>::bits_to_flow(
    const std::bitset<FLOWKEY_BITS>& bits) const {
    FlowKeyType flow{};
    uint8_t* bytes = reinterpret_cast<uint8_t*>(&flow);

    for (size_t i = 0; i < FLOWKEY_SIZE; ++i) {
        uint8_t byte = 0;
        for (size_t j = 0; j < 8; ++j) {
            if (bits[i * 8 + j]) {
                byte |= (1 << j);
            }
        }
        bytes[i] = byte;
    }

    return flow;
}

template <typename FlowKeyType, typename SFINAE>
std::vector<std::bitset<SketchLearn<FlowKeyType, SFINAE>::FLOWKEY_BITS>>
SketchLearn<FlowKeyType, SFINAE>::expand_template(
    const std::string& template_str) const {
    std::vector<std::bitset<FLOWKEY_BITS>> results;
    std::queue<std::string> queue;
    queue.push(template_str);

    // 去重 set
    std::set<std::string> unique_set;

    while (!queue.empty()) {
        std::string current = queue.front();
        queue.pop();

        bool found_wildcard = false;
        for (size_t i = 0; i < current.length(); i++) {
            if (current[i] == '*') {
                // 展开为 0 和 1
                std::string with_0 = current;
                std::string with_1 = current;
                with_0[i] = '0';
                with_1[i] = '1';
                queue.push(with_0);
                queue.push(with_1);
                found_wildcard = true;
                break;
            }
        }

        if (!found_wildcard) {
            unique_set.insert(current);
        }
    }

    // 去重后转换为 bitset
    for (const std::string& str : unique_set) {
        results.push_back(std::bitset<FLOWKEY_BITS>(str));
    }

    return results;
}

template <typename FlowKeyType, typename SFINAE>
void SketchLearn<FlowKeyType, SFINAE>::compute_distribution() {
    // 计算每层的均值和方差
    for (size_t k = 0; k <= FLOWKEY_BITS; k++) {
        const auto& data = layers[k]->get_raw_data();

        // 计算均值
        uint64_t sum = 0;
        uint64_t count = 0;
        for (const auto& row : data) {
            for (const auto& val : row) {
                sum += val;
                count++;
            }
        }
        p[k] = (count > 0) ? (static_cast<double>(sum) / count) : 0.0;

        // 计算方差
        double v = 0.0;
        for (const auto& row : data) {
            for (const auto& val : row) {
                double diff = val - p[k];
                v += diff * diff;
            }
        }
        variance[k] = (count > 0) ? (v / count) : 0.0;
    }
}

template <typename FlowKeyType, typename SFINAE>
std::vector<FlowKeyType> SketchLearn<FlowKeyType, SFINAE>::extract_large_flows(
    uint64_t row_index,
    uint64_t col_index) {
    std::vector<FlowKeyType> candidates;
    std::vector<std::bitset<FLOWKEY_BITS>> candidate_bits;

    // Step 1: 估计位级概率
    std::vector<double> p_(FLOWKEY_BITS + 1, 0.0);
    std::vector<double> Rij(FLOWKEY_BITS + 1, 0.0);

    uint32_t count_0 = layers[0]->get_raw_data()[row_index][col_index];
    if (count_0 == 0) {
        return candidates;
    }

    for (size_t k = 1; k <= FLOWKEY_BITS; k++) {
        uint32_t count_k = layers[k]->get_raw_data()[row_index][col_index];
        Rij[k] = static_cast<double>(count_k) / count_0;

        if (Rij[k] < theta) {
            p_[k] = 0.0;
        } else if ((1.0 - Rij[k]) < theta) {
            p_[k] = 1.0;
        } else {
            // 贝叶斯推断
            p_[k] = 1.0 - Rij[k];
        }
    }

    // Step 2: 找到候选流
    std::string T;
    for (size_t k = 1; k <= FLOWKEY_BITS; k++) {
        if (p_[k] > 0.99) {
            T += '1';
        } else if ((1.0 - p_[k]) > 0.99) {
            T += '0';
        } else {
            T += '*';
        }
    }

    // 展开模板
    std::vector<std::bitset<FLOWKEY_BITS>> expanded_bits = expand_template(T);

    // 验证哈希匹配
    for (const auto& bits : expanded_bits) {
        FlowKeyType flow = bits_to_flow(bits);
        uint64_t hash_index = hash_function->hash(flow, row_index, num_cols);

        if (hash_index == col_index) {
            candidates.push_back(flow);
            candidate_bits.push_back(bits);
        }
    }

    if (candidates.empty()) {
        return candidates;
    }

    // Step 3: 估计频率
    std::vector<double> e;

    for (size_t m = 0; m < candidates.size(); m++) {
        const std::bitset<FLOWKEY_BITS>& f_bits = candidate_bits[m];
        for (size_t k = 1; k <= FLOWKEY_BITS; k++) {
            double e_k = 0.0;

            if (f_bits[FLOWKEY_BITS - k]) {  // 第 k 位是 1
                if ((1.0 - p[k]) > 0) {
                    e_k = ((Rij[k] - p[k]) / (1.0 - p[k])) * count_0;
                    e.push_back(e_k);
                }
            } else {  // 第 k 位是 0
                if (p[k] > 0) {
                    e_k = (1.0 - (Rij[k] / p[k])) * count_0;
                    e.push_back(e_k);
                }
            }
        }
    }

    // 取中位数作为频率估计
    double s_f = count_0;
    if (!e.empty()) {
        std::sort(e.begin(), e.end());
        size_t mid = e.size() / 2;
        if (e.size() % 2 == 0) {
            s_f = (e[mid - 1] + e[mid]) / 2.0;
        } else {
            s_f = e[mid];
        }
    }

    // Step 4: 验证候选流
    std::vector<FlowKeyType> verified_candidates;

    for (size_t m = 0; m < candidates.size(); m++) {
        const FlowKeyType& f = candidates[m];
        const std::bitset<FLOWKEY_BITS>& f_bits = candidate_bits[m];
        double min_s_f = s_f;

        // 遍历所有其他行进行交叉验证
        for (uint64_t i = 0; i < num_rows; i++) {
            if (i == row_index)
                continue;

            uint64_t j = hash_function->hash(f, i, num_cols);

            for (size_t k = 1; k <= FLOWKEY_BITS; k++) {
                uint32_t candidate_count_0 = layers[0]->get_raw_data()[i][j];
                uint32_t candidate_count_k = layers[k]->get_raw_data()[i][j];

                if (!f_bits[FLOWKEY_BITS - k]) {  // 第 k 位是 0
                    double diff = candidate_count_0 - candidate_count_k;
                    min_s_f = std::min(min_s_f, diff);
                } else {  // 第 k 位是 1
                    min_s_f = std::min(min_s_f,
                                       static_cast<double>(candidate_count_k));
                }
            }
        }

        // 通过验证的候选流
        if (min_s_f >= theta * count_0) {
            verified_candidates.push_back(f);
        }
    }

    return verified_candidates;
}

template <typename FlowKeyType, typename SFINAE>
void SketchLearn<FlowKeyType, SFINAE>::update(const FlowKeyType& flow,
                                              int increment) {
    std::bitset<FLOWKEY_BITS> bits = flow_to_bits(flow);

    // 更新 Level 0
    layers[0]->update(flow, increment);

    // 更新 Level k（第 k 位为 1 的流）
    for (size_t k = 1; k <= FLOWKEY_BITS; k++) {
        if (bits[FLOWKEY_BITS - k]) {
            layers[k]->update(flow, increment);
        }
    }

    // 更新后需要重新解码
    is_decoded = false;
}

template <typename FlowKeyType, typename SFINAE>
std::map<FlowKeyType, uint64_t> SketchLearn<FlowKeyType, SFINAE>::decode() {
    std::map<FlowKeyType, uint64_t> result;

    // 计算位级计数器分布
    compute_distribution();

    // 对每个 (row, col) 提取大流
    std::vector<FlowKeyType> all_candidates;
    for (uint64_t i = 0; i < num_rows; i++) {
        for (uint64_t j = 0; j < num_cols; j++) {
            std::vector<FlowKeyType> candidates = extract_large_flows(i, j);
            all_candidates.insert(all_candidates.end(), candidates.begin(),
                                  candidates.end());
        }
    }

    // 将候选流加入结果
    for (const auto& flow : all_candidates) {
        uint64_t estimated_count = layers[0]->query(flow);
        if (estimated_count > 0) {
            result[flow] = estimated_count;
        }
    }

    // 从其他统计层中减去提取出的大流
    for (const auto& pair : result) {
        const FlowKeyType& flow = pair.first;
        uint32_t count = static_cast<uint32_t>(pair.second);

        std::bitset<FLOWKEY_BITS> bits = flow_to_bits(flow);

        // 从 Level 0 减去
        for (uint64_t i = 0; i < num_rows; i++) {
            auto& data_0 = const_cast<std::vector<std::vector<uint32_t>>&>(
                layers[0]->get_raw_data());
            uint64_t index_0 = hash_function->hash(flow, i, num_cols);
            if (data_0[i][index_0] >= count) {
                data_0[i][index_0] -= count;
            } else {
                data_0[i][index_0] = 0;
            }
        }

        // 从 Level k 减去
        for (size_t k = 1; k <= FLOWKEY_BITS; k++) {
            if (bits[FLOWKEY_BITS - k]) {
                for (uint64_t i = 0; i < num_rows; i++) {
                    auto& data_k =
                        const_cast<std::vector<std::vector<uint32_t>>&>(
                            layers[k]->get_raw_data());
                    uint64_t index_k = hash_function->hash(flow, i, num_cols);
                    if (data_k[i][index_k] >= count) {
                        data_k[i][index_k] -= count;
                    } else {
                        data_k[i][index_k] = 0;
                    }
                }
            }
        }
    }

    // 重新计算分布
    compute_distribution();

    // 缓存结果
    decoded_map = result;
    is_decoded = true;

    return result;
}

template <typename FlowKeyType, typename SFINAE>
uint64_t SketchLearn<FlowKeyType, SFINAE>::query(const FlowKeyType& flow) {
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

template class SketchLearn<OneTuple>;
template class SketchLearn<TwoTuple>;
template class SketchLearn<FiveTuple>;
