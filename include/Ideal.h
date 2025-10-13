#ifndef IDEAL_H
#define IDEAL_H

#include <cstddef>
#include <unordered_map>

#include "Sketch.h"
#include "TwoTuple.h"

class Ideal : public Sketch {
   private:
    std::unordered_map<TwoTuple, uint64_t, TwoTupleHash> flow_counter;

   public:
    Ideal() = default;
    ~Ideal() = default;

    void update(const TwoTuple& flow, int increment = 1) override;
    uint64_t query(const TwoTuple& flow) override;

    inline size_t get_flow_count() const { return flow_counter.size(); }

    inline void clear() override { flow_counter.clear(); }

    inline const std::unordered_map<TwoTuple, uint64_t, TwoTupleHash>&
    get_raw_data() {
        return flow_counter;
    }
};

#endif /* IDEAL_H */
