# SketchLib - 流式数据草图算法库

SketchLib 是一个 C++ 流式数据草图算法库，用于网络流量监控和频率估计。

## 📚 Sketch 算法

### Count-Min Sketch
Count-Min Sketch 使用多个哈希函数将流映射到计数器矩阵，查询时返回所有哈希位置的最小值。适用于频率估计和重流检测，永远不会低估频率。

### Count Sketch
Count Sketch 使用有符号计数器（+1/-1）和中位数聚合，提供无偏估计。适用于需要准确估计的场景。

### Sample-and-Hold
Sample-and-Hold 维护一个容量有限的精确哈希表，满时驱逐最小计数的流。提供精确计数（无误差），适合重流追踪。

### UnivMon
UnivMon 是多分辨率监控框架，使用分层采样（每层采样概率递减）。支持 Sample-and-Hold 和 Count Sketch 两种后端，适合多尺度流量分析。

### ElasticSketch
ElasticSketch 采用双层架构：Heavy Part 使用投票机制精确记录大流，Light Part 使用 Count-Min Sketch 近似记录小流。自适应识别流应当在Heavy Part 还是 Light Part 并从Heavy Part 驱逐小流到 Light Part。

### HashPipe
HashPipe 使用多级流水线结构，大流会沉淀在某一级并精确记录，小流逐级推进最终被过滤。适合只关心大流、不在乎小流的场景。

## 🔧 构建说明

### 前置要求
- CMake 3.16+
- C++14 编译器（GCC 5+, Clang 3.4+, MSVC 2015+）
- （可选）Ninja 构建系统

### 使用 CMake + Ninja 构建

#### Windows (PowerShell)
```powershell
# 创建并进入构建目录
mkdir build
cd build

# 使用 Ninja 生成构建文件
cmake -G Ninja ..

# 构建项目
ninja

# 运行测试
.\tests\sketch_tests.exe

# 运行示例
.\examples\example.exe
```

#### Linux / macOS
```bash
# 创建并进入构建目录
mkdir build
cd build

# 使用 Ninja 生成构建文件
cmake -G Ninja ..

# 构建项目
ninja

# 运行测试
./tests/sketch_tests

# 运行示例
./examples/example
```

### 使用 CMake + Make 构建（不使用 Ninja）

#### Windows
```powershell
mkdir build
cd build
cmake ..
cmake --build .
```

#### Linux / macOS
```bash
mkdir build
cd build
cmake ..
make
```

### CMake 构建选项

```bash
# 只构建库，不构建示例
cmake -DBUILD_EXAMPLES=OFF ..

# 只构建库，不构建测试
cmake -DBUILD_TESTS=OFF ..

# 指定编译器
cmake -DCMAKE_CXX_COMPILER=g++ ..

# Release 模式（优化编译）
cmake -DCMAKE_BUILD_TYPE=Release ..

# Debug 模式（调试信息）
cmake -DCMAKE_BUILD_TYPE=Debug ..
```

## 📁 项目结构

```
SketchLib/
├── CMakeLists.txt              # 主 CMake 配置
├── README.md                   # 本文件
│
├── include/                    # 公共头文件
│   ├── Sketch.h                # Sketch 基类
│   ├── TwoTuple.h              # 流标识符
│   ├── HashFunction.h          # 哈希函数接口
│   ├── CountMin.h              # Count-Min Sketch
│   ├── CountSketch.h           # Count Sketch
│   ├── SampleAndHold.h         # Sample-and-Hold
│   ├── UnivMon.h               # UnivMon
│   ├── ElasticSketch.h         # ElasticSketch
│   ├── HashPipe.h              # HashPipe
│   └── seed_list.h             # 哈希种子列表
│
├── src/                        # 源文件实现
│   ├── CountMin.cpp
│   ├── CountSketch.cpp
│   ├── SampleAndHold.cpp
│   ├── UnivMon.cpp
│   ├── ElasticSketch.cpp
│   ├── HashPipe.cpp
│   ├── HashFunction.cpp
│   └── seed_list.cpp
│
├── third_party/                # 第三方库
│   ├── doctest.h               # 测试框架
│   ├── crc64.h/c               # Redis CRC64 哈希函数
│   ├── MurmurHash3.h/cpp       # MurmurHash3 哈希函数
│   └── SpookyV2.h/cpp          # SpookyHash 哈希函数
│
├── tests/                      # 测试代码
│   ├── CMakeLists.txt
│   ├── test_main.cpp           # 测试主函数
│   ├── test_countmin.cpp       # Count-Min 测试
│   ├── test_countsketch.cpp    # Count Sketch 测试
│   ├── test_sampleandhold.cpp  # Sample-and-Hold 测试
│   ├── test_univmon.cpp        # UnivMon 测试
│   ├── test_elasticsketch.cpp  # ElasticSketch 测试
│   └── test_hashpipe.cpp       # HashPipe 测试
│
├── examples/                   # 示例代码
│   ├── CMakeLists.txt
│   └── example.cpp             # 综合示例
│
└── build/                      # 构建输出（生成）
    ├── libSketchLib.a          # 静态库
    ├── tests/sketch_tests      # 测试程序
    └── examples/example        # 示例程序
```

## 🙏 致谢

本项目使用了以下优秀的第三方库：

- **[BMv2 CRC32](https://github.com/p4lang/behavioral-model)** - BMv2 中的 CRC32 实现
- **[Redis CRC64](https://github.com/redis/redis)** - Redis 5.0 中的 CRC64 实现，采用 Jones 多项式
- **[MurmurHash3](https://github.com/aappleby/smhasher)** - Austin Appleby 开发的高性能非加密哈希函数
- **[SpookyHash](http://burtleburtle.net/bob/hash/spooky.html)** - Bob Jenkins 开发的快速哈希函数
- **[doctest](https://github.com/doctest/doctest)** - Viktor Kirilov 开发的轻量级 C++ 测试框架

感谢这些开源项目为社区做出的贡献！
