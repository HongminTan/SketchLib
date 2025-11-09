# SketchLib 示例程序

本目录包含 SketchLib 的 7 个核心 Sketch 算法的最简使用示例。

## 📁 示例文件

| 文件名 | Sketch 类型 | 说明 |
|--------|------------|------|
| `example_countmin.cpp` | Count-Min Sketch | 频率估计，只会高估 |
| `example_countsketch.cpp` | Count Sketch | 无偏估计，支持负增量 |
| `example_elasticsketch.cpp` | ElasticSketch | 重流精确计数，轻流近似 |
| `example_hashpipe.cpp` | HashPipe | 多级管道，自动保留重流 |
| `example_univmon.cpp` | UnivMon | 分层采样，多分辨率监控 |
| `example_sketchlearn.cpp` | SketchLearn | 位级分层，主动发现大流 |
| `example_flowradar.cpp` | FlowRadar | XOR 编码，迭代解码 |

## 🎯 FlowKey 类型

每个示例都演示了三种 FlowKey 类型的使用：

- **OneTuple**: 单个 IP 地址
- **TwoTuple**: 源 IP - 目的 IP 对
- **FiveTuple**: 完整五元组（源 IP、目的 IP、源端口、目的端口、协议）

## 🚀 编译示例

### 前置条件

确保 SketchLib 已经编译完成。

### 编译所有示例

```bash
cd build
cmake -G Ninja -DBUILD_EXAMPLES=ON ..
ninja
```

### 编译单个示例

```bash
cd build
ninja example_countmin
ninja example_countsketch
ninja example_elasticsketch
ninja example_hashpipe
ninja example_univmon
ninja example_sketchlearn
ninja example_flowradar
```

## 🏃 运行示例

编译后的可执行文件位于 `build/examples/` 目录：

```bash
# 从 build 目录运行
./examples/example_countmin
./examples/example_countsketch
./examples/example_elasticsketch
./examples/example_hashpipe
./examples/example_univmon
./examples/example_sketchlearn
./examples/example_flowradar
```

## 📊 示例输出

每个示例都会输出带标签的查询结果和配置参数：

### Count-Min Sketch
```
query=10 rows=4 cols=64
query=5 rows=4 cols=64
query=7 rows=4 cols=128
```

### Count Sketch
```
query=10 rows=5 cols=102
query=2 rows=5 cols=102
query=7 rows=5 cols=204
```

### ElasticSketch
```
query=100 heavy_buckets=62 light_rows=8 light_cols=96 lambda=2
query=100 heavy_buckets=62 light_rows=8 light_cols=96 lambda=2
query=100 heavy_buckets=125 light_rows=8 light_cols=192 lambda=2
```

### HashPipe
```
query=25 stages=4 buckets_per_stage=85
query=35 stages=4 buckets_per_stage=85
query=10 stages=4 buckets_per_stage=170
```

### UnivMon
```
query=200 layer_count=3 memory_budget=4096 backend=1
query=100 layer_count=3 memory_budget=4096 backend=1
query=100 layer_count=3 memory_budget=8192 backend=1
```

### SketchLearn
```
query=10 rows=2 cols=512 theta=0.5
query=5 rows=2 cols=512
query=7
```

### FlowRadar
```
query=10 bf_hashes=3 ct_hashes=3 table_size=145
query=5 bf_hashes=3 ct_hashes=3 table_size=145
query=7 bf_hashes=4 ct_hashes=3 table_size=290
```

## 💡 代码结构

每个示例文件的结构：

```cpp
#include <iostream>
#include "FlowKey.h"
#include "SketchName.h"

int main() {
    // OneTuple 示例
    {
        SketchName<OneTuple> sketch(params...);
        OneTuple key(0x01020304);
        sketch.update(key, value);
        std::cout << "query=" << sketch.query(key) << " ..." << std::endl;
    }
    
    // TwoTuple 示例
    {
        SketchName<TwoTuple> sketch(params...);
        TwoTuple key(src_ip, dst_ip);
        sketch.update(key, value);
        std::cout << "query=" << sketch.query(key) << " ..." << std::endl;
    }
    
    // FiveTuple 示例
    {
        SketchName<FiveTuple> sketch(params...);
        FiveTuple key(src_ip, dst_ip, src_port, dst_port, protocol);
        sketch.update(key, value);
        std::cout << "query=" << sketch.query(key) << " ..." << std::endl;
    }
    
    return 0;
}
```

## 📖 API 说明

### 基本操作

所有 Sketch 都支持以下基本操作：

```cpp
// 创建 Sketch
SketchName<FlowKeyType> sketch(constructor_params...);

// 更新流计数
sketch.update(flow_key, increment);

// 查询流计数
uint64_t count = sketch.query(flow_key);
```

### 配置查询

不同 Sketch 提供不同的配置查询函数：

- **Count-Min / Count Sketch**: `get_rows()`, `get_cols()`
- **ElasticSketch**: `get_heavy_bucket_count()`, `get_light_size()`, `get_lambda()`
- **HashPipe**: `get_num_stages()`, `get_buckets_per_stage()`
- **UnivMon**: `get_layer_count()`, `get_memory_budget()`, `get_backend()`
- **SketchLearn**: `get_num_rows()`, `get_num_cols()`, `get_theta()`
- **FlowRadar**: `get_bf_num_hashes()`, `get_ct_num_hashes()`, `get_table_size()`

## 📚 更多信息

- 完整 API 文档：查看 `include/` 目录下的头文件
- 项目主页：[SketchLib GitHub](https://github.com/HongminTan/SketchLib)
