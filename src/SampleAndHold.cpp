#include "SampleAndHold.h"

SampleAndHold::SampleAndHold(uint64_t capacity_value)
    : counters(), capacity(capacity_value > 0 ? capacity_value : 1) {
    counters.reserve(static_cast<std::size_t>(capacity));
}

void SampleAndHold::update(const TwoTuple& flow, int increment) {
    if (increment <= 0) {
        return;
    }

    uint64_t inc = static_cast<uint64_t>(increment);
    auto it = counters.find(flow);
    if (it != counters.end()) {
        it->second += inc;
        return;
    }

    if (counters.size() < capacity) {
        counters.emplace(flow, inc);
        return;
    }

    auto min_it = find_min();
    if (min_it != counters.end() && inc > min_it->second) {
        counters.erase(min_it);
        counters.emplace(flow, inc);
    }
}

uint64_t SampleAndHold::query(const TwoTuple& flow) {
    auto it = counters.find(flow);
    if (it == counters.end()) {
        return 0;
    }
    return it->second;
}

SampleAndHold::CounterMap::iterator SampleAndHold::find_min() {
    auto min_it = counters.begin();
    for (auto it = counters.begin(); it != counters.end(); ++it) {
        if (it->second < min_it->second) {
            min_it = it;
        }
    }
    return min_it;
}
