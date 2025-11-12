#include "Ideal.h"

template <typename FlowKeyType, typename SFINAE>
void Ideal<FlowKeyType, SFINAE>::update(const FlowKeyType& flow,
                                        int increment) {
    flow_counter[flow] += increment;
}

template <typename FlowKeyType, typename SFINAE>
uint64_t Ideal<FlowKeyType, SFINAE>::query(const FlowKeyType& flow) const {
    auto it = flow_counter.find(flow);
    if (it != flow_counter.end()) {
        return it->second;
    }
    return 0;
}

template class Ideal<OneTuple>;
template class Ideal<TwoTuple>;
template class Ideal<FiveTuple>;
