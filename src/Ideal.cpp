#include "Ideal.h"

void Ideal::update(const TwoTuple& flow, int increment) {
    flow_counter[flow] += increment;
}

uint64_t Ideal::query(const TwoTuple& flow) {
    auto it = flow_counter.find(flow);
    if (it != flow_counter.end()) {
        return it->second;
    }
    return 0;
}
