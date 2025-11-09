#ifndef SKETCH_H
#define SKETCH_H

#include <cstdint>

#include "FlowKey.h"

/**
 * @brief 所有 sketch 的模板基类
 *
 * @tparam FlowKeyType 流标识符类型（OneTuple, TwoTuple, FiveTuple）
 * @tparam SFINAE 模板类型约束，确保只接受有效的 FlowKey 类型，否则会推导失败
 */
template <typename FlowKeyType, typename SFINAE = RequireFlowKey<FlowKeyType>>
class Sketch {
   public:
    /**
     * @brief 用一个流更新 sketch
     * @param flow 流标识符
     * @param inc 增量值
     */
    virtual void update(const FlowKeyType& flow, int inc) = 0;

    /**
     * @brief 查询流的估计计数
     * @param flow 流标识符
     * @return 估计计数
     */
    virtual uint64_t query(const FlowKeyType& flow) = 0;

    /**
     * @brief 检查流是否存在于 sketch 中
     * @param flow 流标识符
     * @return 如果观察到该流则返回 true，否则返回 false
     */
    virtual bool has_flow(const FlowKeyType& flow) { return query(flow) > 0; }

    /**
     * @brief 清空 sketch 的内容
     */
    virtual void clear() = 0;

    virtual ~Sketch() = default;
};

#endif  // SKETCH_H