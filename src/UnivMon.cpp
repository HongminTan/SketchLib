#include "UnivMon.h"

template <typename FlowKeyType, typename SFINAE>
UnivMon<FlowKeyType, SFINAE>::UnivMon(
    uint64_t num_layers_value,
    uint64_t total_memory_bytes_value,
    std::unique_ptr<HashFunction<FlowKeyType>> hash_function_value,
    UnivMonBackend backend_value)
    : num_layers(num_layers_value > 0 ? num_layers_value : 1),
      total_memory_bytes(total_memory_bytes_value > 0 ? total_memory_bytes_value
                                                      : 1),
      backend(backend_value),
      hash_function(std::move(hash_function_value)),
      layers(),
      rng(std::random_device{}()) {
    if (!hash_function) {
        hash_function = std::make_unique<DefaultHashFunction<FlowKeyType>>();
    }
    if (total_memory_bytes < num_layers) {
        total_memory_bytes = num_layers;
    }
    initialize_layers();
}

template <typename FlowKeyType, typename SFINAE>
UnivMon<FlowKeyType, SFINAE>::UnivMon(const UnivMon& other)
    : num_layers(other.num_layers),
      total_memory_bytes(other.total_memory_bytes),
      backend(other.backend),
      hash_function(other.hash_function
                        ? other.hash_function->clone()
                        : std::make_unique<DefaultHashFunction<FlowKeyType>>()),
      layers(),
      rng(std::random_device{}()) {
    layers.reserve(static_cast<std::size_t>(num_layers));
    for (const auto& layer : other.layers) {
        if (layer) {
            if (auto cs =
                    dynamic_cast<CountSketch<FlowKeyType>*>(layer.get())) {
                layers.push_back(
                    std::make_unique<CountSketch<FlowKeyType>>(*cs));
            } else if (auto sah = dynamic_cast<SampleAndHold<FlowKeyType>*>(
                           layer.get())) {
                layers.push_back(
                    std::make_unique<SampleAndHold<FlowKeyType>>(*sah));
            } else {
                std::cerr << "Unknown Sketch type during copy." << std::endl;
                exit(1);
            }
        } else {
            layers.push_back(nullptr);
        }
    }
}

template <typename FlowKeyType, typename SFINAE>
UnivMon<FlowKeyType, SFINAE>& UnivMon<FlowKeyType, SFINAE>::operator=(
    const UnivMon& other) {
    if (this != &other) {
        num_layers = other.num_layers;
        total_memory_bytes = other.total_memory_bytes;
        backend = other.backend;
        hash_function =
            other.hash_function
                ? other.hash_function->clone()
                : std::make_unique<DefaultHashFunction<FlowKeyType>>();
        layers.clear();
        layers.reserve(static_cast<std::size_t>(num_layers));
        for (const auto& layer : other.layers) {
            if (layer) {
                if (auto cs =
                        dynamic_cast<CountSketch<FlowKeyType>*>(layer.get())) {
                    layers.push_back(
                        std::make_unique<CountSketch<FlowKeyType>>(*cs));
                } else if (auto sah = dynamic_cast<SampleAndHold<FlowKeyType>*>(
                               layer.get())) {
                    layers.push_back(
                        std::make_unique<SampleAndHold<FlowKeyType>>(*sah));
                } else {
                    std::cerr << "Unknown Sketch type during copy."
                              << std::endl;
                    exit(1);
                }
            } else {
                layers.push_back(nullptr);
            }
        }
        rng.seed(std::random_device{}());
    }
    return *this;
}

template <typename FlowKeyType, typename SFINAE>
void UnivMon<FlowKeyType, SFINAE>::update(const FlowKeyType& flow,
                                          int increment) {
    if (increment <= 0) {
        return;
    }

    std::uniform_real_distribution<double> distribution(0.0, 1.0);
    for (uint64_t layer_index = 0; layer_index < num_layers; ++layer_index) {
        double probability = sample_probability(layer_index);
        double sample = distribution(rng);
        if (sample <= probability) {
            layers[layer_index]->update(flow, increment);
        } else {
            break;
        }
    }
}

template <typename FlowKeyType, typename SFINAE>
uint64_t UnivMon<FlowKeyType, SFINAE>::query(const FlowKeyType& flow) {
    uint64_t best = 0;

    // 论文说取全部非零层的最大值
    for (uint64_t layer_index = 0; layer_index < num_layers; ++layer_index) {
        if (!layers[layer_index]->has_flow(flow)) {
            continue;
        }
        uint64_t observed = layers[layer_index]->query(flow);
        if (observed == 0) {
            continue;
        }
        uint64_t estimate = scale_observation(observed, layer_index);
        if (estimate > best) {
            best = estimate;
        }
    }

    return best;
}

template <typename FlowKeyType, typename SFINAE>
void UnivMon<FlowKeyType, SFINAE>::initialize_layers() {
    layers.clear();
    layers.reserve(static_cast<std::size_t>(num_layers));

    uint64_t base_memory = total_memory_bytes / num_layers;
    uint64_t remainder = total_memory_bytes % num_layers;

    for (uint64_t layer_index = 0; layer_index < num_layers; ++layer_index) {
        uint64_t layer_memory = base_memory + (layer_index < remainder ? 1 : 0);
        if (layer_memory == 0) {
            layer_memory = 1;
        }
        switch (backend) {
            case UnivMonBackend::SaH: {
                uint64_t entry_bytes = sizeof(FlowKeyType) + sizeof(uint64_t);
                if (entry_bytes == 0) {
                    entry_bytes = 16;
                }
                uint64_t capacity = layer_memory / entry_bytes;
                if (capacity == 0) {
                    capacity = 1;
                }
                layers.push_back(
                    std::make_unique<SampleAndHold<FlowKeyType>>(capacity));
                break;
            }
            case UnivMonBackend::CountSketch: {
                uint64_t num_hashes = 8;
                layers.push_back(std::make_unique<CountSketch<FlowKeyType>>(
                    num_hashes, layer_memory));
                break;
            }
            default: {
                std::cerr << "Unknown Backend Type" << std::endl;
                exit(1);
            }
        }
    }
}

template <typename FlowKeyType, typename SFINAE>
double UnivMon<FlowKeyType, SFINAE>::sample_probability(uint64_t layer) const {
    if (layer == 0) {
        return 1.0;
    }
    if (layer >= 63) {
        return std::pow(0.5, static_cast<double>(layer));
    }
    return 1.0 / static_cast<double>(1ULL << layer);
}

template <typename FlowKeyType, typename SFINAE>
uint64_t UnivMon<FlowKeyType, SFINAE>::scale_observation(uint64_t observed,
                                                         uint64_t layer) const {
    if (observed == 0) {
        return 0;
    }
    if (layer >= 63) {
        return std::numeric_limits<uint64_t>::max();
    }
    uint64_t factor = 1ULL << layer;
    if (observed > std::numeric_limits<uint64_t>::max() / factor) {
        return std::numeric_limits<uint64_t>::max();
    }
    return observed * factor;
}

template class UnivMon<OneTuple>;
template class UnivMon<TwoTuple>;
template class UnivMon<FiveTuple>;
