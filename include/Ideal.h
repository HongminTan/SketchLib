#ifndef IDEAL_H
#define IDEAL_H

#include <cstddef>
#include <unordered_map>

#include "FlowKey.h"
#include "Sketch.h"

template <typename FlowKeyType, typename SFINAE = RequireFlowKey<FlowKeyType>>
class Ideal : public Sketch<FlowKeyType> {
   private:
    std::unordered_map<FlowKeyType, uint64_t> flow_counter;

   public:
    Ideal() = default;
    ~Ideal() = default;

    void update(const FlowKeyType& flow, int increment = 1) override;
    uint64_t query(const FlowKeyType& flow) override;

    inline size_t get_flow_count() const { return flow_counter.size(); }

    inline void clear() override { flow_counter.clear(); }

    inline const std::unordered_map<FlowKeyType, uint64_t>& get_raw_data() {
        return flow_counter;
    }
};

#endif /* IDEAL_H */
