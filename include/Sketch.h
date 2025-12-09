#ifndef SKETCH_H
#define SKETCH_H

#include <cstdint>

#include "FlowKey.h"

class Sketch {
   public:
    /**
     * @brief 查询流的估计计数
     * @param flow 流标识符
     * @return 估计计数
     */
    virtual uint64_t query(const FlowKeyType& flow) const = 0;

    /**
     * @brief 检查流是否存在于 sketch 中
     * @param flow 流标识符
     * @return 如果观察到该流则返回 true，否则返回 false
     */
    virtual bool has_flow(const FlowKeyType& flow) const {
        return query(flow) > 0;
    }

    /**
     * @brief 清空 sketch 的内容
     */
    virtual void clear() = 0;

    virtual ~Sketch() = default;
};

#endif  // SKETCH_H