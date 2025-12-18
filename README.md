# SketchEBPFLib

SketchEBPFLib 是一个基于 eBPF 的高性能流式数据 Sketch 算法库，可用于网络流量监控和频率估计。通过将 Sketch 算法卸载到内核态 XDP 层，实现零拷贝、低延迟的流量统计。

## 📚 Sketch 算法

### Count-Min Sketch
Count-Min Sketch 使用多个哈希函数将流映射到计数器矩阵，查询时返回所有哈希位置的最小值。适用于频率估计和重流检测，永远不会低估频率。

### Count Sketch
Count Sketch 使用有符号计数器（+1/-1）和中位数聚合，提供无偏估计。适用于需要准确估计的场景。

### ElasticSketch
ElasticSketch 采用双层架构：Heavy Part 使用投票机制精确记录大流，Light Part 使用 Count-Min Sketch 近似记录小流。自适应识别流应当在 Heavy Part 还是 Light Part 并从 Heavy Part 驱逐小流到 Light Part。

### MV-Sketch
MV-Sketch 使用投票机制识别主要流的频率估计算法。每个桶存储一个候选流，通过 count 的正负来识别主要流。查询时使用多行哈希取最小值来减少估计误差。适合内存受限场景下的流频率估计。

### FlowRadar
FlowRadar 基于 XOR 编码和迭代解码，可以恢复所有流及其精确频率。适合需要完整流信息的离线分析场景。

## 🔧 开发环境搭建

### 系统要求
- Debian/Ubuntu Linux（内核版本 5.10+）
- 支持 eBPF 和 XDP 的网卡（或者使用 scripts/network.sh 创建的虚拟环境）

### 安装依赖

#### 1. 安装 Clang 编译器
```bash
# 安装 Clang 和 LLVM（需要 Clang 20+）
sudo apt-get update
wget https://apt.llvm.org/llvm.sh
chmod +x llvm.sh
sudo ./llvm.sh 20
sudo apt-get install -y libelf-dev zlib1g-dev clang-20 llvm-20 llvm-20-dev lld-20 libbpf-dev ninja-build
sudo update-alternatives --install /usr/bin/clang clang /usr/bin/clang-20 200
sudo update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-20 200
sudo update-alternatives --install /usr/bin/llvm-config llvm-config /usr/bin/llvm-config-20 200
sudo update-alternatives --install /usr/bin/llvm-strip llvm-strip /usr/bin/llvm-strip-20 200
sudo update-alternatives --install /usr/bin/lld lld /usr/bin/lld-20 200

# 验证安装
clang --version
```

#### 2. 安装 eBPF 开发依赖
```bash
# 安装 libbpf 开发库和内核头文件
sudo apt install -y libbpf-dev linux-headers-$(uname -r)

# 安装其他必备工具
sudo apt install -y libelf-dev
```

#### 3. 自行编译 bpftool
apt 安装的 bpftool 不支持 `bpftool prog profile` 功能，需要从内核源码编译：

```bash
# 安装编译依赖
sudo apt install -y build-essential libelf-dev libcap-dev pkg-config binutils-dev

# 下载内核源码
cd /tmp
git clone --depth 1 https://github.com/torvalds/linux.git
cd linux/tools/bpf/bpftool

# 编译 bpftool
make

# 安装到系统路径
sudo make install

# 验证安装
bpftool --version
bpftool prog help | grep profile
```

#### 4. 安装 CMake 和 Ninja
```bash
sudo apt install -y cmake ninja-build
```

### 构建项目

```bash
# 创建并进入构建目录
mkdir -p build
cd build

# 使用 CMake 生成构建文件
cmake -G Ninja ..

# 构建项目
ninja

# 运行主程序
ls -l | grep SketchEBPFLib
```

## 📁 项目结构

```
SketchLib/
├── CMakeLists.txt              # CMake 构建配置
├── README.md                   # 本文件
│
├── include/                    # 公共头文件
│   ├── Sketch.h                # Sketch 基类接口
│   ├── FlowKey.h               # 流标识符（OneTuple/TwoTuple/FiveTuple）
│   ├── hash.h                  # 哈希函数接口
│   ├── Config.h                # 全局配置（内存大小、行列数等）
│   ├── CountMin.h              # Count-Min Sketch 用户态接口
│   ├── CountSketch.h           # Count Sketch 用户态接口
│   ├── ElasticSketch.h         # ElasticSketch 用户态接口
│   ├── MVSketch.h              # MV-Sketch 用户态接口
│   ├── FlowRadar.h             # FlowRadar 用户态接口
│   ├── seed_list.h             # 哈希种子列表
│   └── autogen/                # 自动生成的头文件（构建时生成）
│       ├── vmlinux.h           # 内核类型定义
│       ├── CountMin.skel.h     # Count-Min Sketch skeleton
│       ├── CountSketch.skel.h  # Count Sketch skeleton
│       ├── ElasticSketch.skel.h # ElasticSketch skeleton
│       ├── MVSketch.skel.h     # MV-Sketch skeleton
│       └── FlowRadar.skel.h    # FlowRadar skeleton
│
├── src/                        # 源文件
│   ├── kernel/                 # 内核态 eBPF 程序
│   │   ├── CountMin.bpf.c      # Count-Min Sketch 内核态实现
│   │   ├── CountSketch.bpf.c   # Count Sketch 内核态实现
│   │   ├── ElasticSketch.bpf.c # ElasticSketch 内核态实现
│   │   ├── MVSketch.bpf.c      # MV-Sketch 内核态实现
│   │   └── FlowRadar.bpf.c     # FlowRadar 内核态实现
│   │
│   ├── user/                   # 用户态程序
│   │   ├── CountMin.cpp        # Count-Min Sketch 用户态实现
│   │   ├── CountSketch.cpp     # Count Sketch 用户态实现
│   │   ├── ElasticSketch.cpp   # ElasticSketch 用户态实现
│   │   ├── MVSketch.cpp        # MV-Sketch 用户态实现
│   │   ├── FlowRadar.cpp       # FlowRadar 用户态实现
│   │   └── main.cpp            # 主程序入口
│   │
│   └── hash.c                  # 哈希函数实现（内核态和用户态共用）
│
├── third_party/                # 第三方库
│   ├── crc32.h/cpp             # BMv2 CRC32 哈希函数
│   └── doctest.h               # 测试框架
│
├── scripts/                    # 辅助脚本
│   └── network.sh              # 网络环境配置脚本
│
└── build/                      # 构建输出
    ├── libSketchEBPFLib.a      # 静态库
    ├── *.bpf.o                 # eBPF 目标文件
    └── *.o                     # 链接后的 eBPF 对象
```

## 🚀 使用方法

### 编译流程说明

SketchEBPFLib 的编译过程涉及多个自动生成的文件，理解这些文件的作用有助于集成到你的项目中。

#### 1. 编译流程概览

```bash
cd build
cmake -G Ninja ..
ninja
```

编译过程会依次生成以下文件：

**第一步：生成 vmlinux.h**
- 位置：`include/autogen/vmlinux.h`
- 作用：包含当前运行内核的所有类型定义（结构体、宏等）
- 生成方式：`bpftool btf dump file /sys/kernel/btf/vmlinux format c > vmlinux.h`
- 用途：eBPF 内核态程序需要包含此文件以访问内核数据结构

**第二步：编译 eBPF 内核态程序**
- 源文件：`src/kernel/*.bpf.c`（如 CountMin.bpf.c）
- 输出：`build/*.bpf.o`（eBPF 字节码目标文件）
- 编译器：Clang with `-target bpf`
- 依赖：vmlinux.h、Config.h、FlowKey.h、hash.h

**第三步：链接通用依赖生成最终 eBPF 对象**
- 输入：`*.bpf.o` + `hash.o`（通用哈希函数）
- 输出：`build/*.o`（链接后的 eBPF 对象）
- 工具：`bpftool gen object`

**第四步：生成 skeleton 头文件**
- 输入：`build/*.o`
- 输出：`include/autogen/*.skel.h`（如 CountMin.skel.h）
- 生成方式：`bpftool gen skeleton *.o > *.skel.h`
- 作用：提供用户态加载和操作 eBPF 程序的 C 接口

**第五步：编译用户态程序**
- 源文件：`src/user/*.cpp`
- 依赖：skeleton 头文件、用户态头文件（CountMin.h 等）
- 链接库：`libbpf`
- 输出：`libSketchEBPFLib.a`（静态库）

#### 2. 集成到你的项目

**方式一：使用静态库**

```cmake
# 在你的 CMakeLists.txt 中
include_directories(/path/to/SketchLib/include)
include_directories(/path/to/SketchLib/include/autogen)

add_executable(main main.cpp)
target_link_libraries(main
    /path/to/SketchLib/build/libSketchEBPFLib.a
    bpf
)
```

**方式二：作为子项目**

```cmake
# 在你的 CMakeLists.txt 中
add_subdirectory(SketchLib)
target_link_libraries(main SketchEBPFLib)
```

#### 3. 需要包含的头文件

```cpp
#include "CountMin.h"      // Count-Min Sketch 用户态接口
#include "CountSketch.h"   // Count Sketch 用户态接口
#include "ElasticSketch.h" // ElasticSketch 用户态接口
#include "MVSketch.h"      // MV-Sketch 用户态接口
#include "FlowRadar.h"     // FlowRadar 用户态接口
```

这些头文件会自动包含所需的 skeleton 头文件和依赖。

#### 4. 配置参数

在 [include/Config.h](include/Config.h) 中可以配置各个 Sketch 的参数：

```c
// Count-Min Sketch 配置
#define CM_ROWS 4                           // 哈希函数数量
#define CM_MEMORY (1 * 1024 * 1024)         // 总内存 1MB
#define CM_COUNTER_TYPE uint32_t            // 计数器类型

// ElasticSketch 配置
#define ES_TOTAL_MEMORY (1 * 1024 * 1024)   // 总内存 1MB
#define ES_HEAVY_MEMORY (256 * 1024)        // Heavy Part 256KB
#define ES_LAMBDA 8                         // 投票替换阈值
```

修改配置后需要重新编译整个项目。

**注意：** Config.h 中的内存大小参数仅为理论值，实际分配的内存受到 double buffer 设计、ebpf 内存对齐、自旋锁保证数据一致性等因素影响，实际使用是定义中的**两倍**以上。

### 运行示例

#### 测试网络环境

SketchLib 提供了 `network.sh` 脚本用于创建虚拟网络环境进行测试。

**网络拓扑：**

```
┌─────────────────────┐     veth pair     ┌─────────────────────┐
│   sender 命名空间    │◄────────────────►│    主命名空间        │
│                     │                   │                     │
│   veth-send         │                   │   veth-recv         │
│   10.10.10.1/24     │                   │   10.10.10.2/24     │
│                     │                   │   (XDP 挂载点)      │
└─────────────────────┘                   └─────────────────────┘
```

**说明：**
- 使用 Linux network namespace 隔离发送端和接收端
- veth pair 是一对虚拟网卡，一端在 sender 命名空间，另一端在主命名空间
- XDP 程序挂载在主命名空间的 `veth-recv` 网卡上
- 从 sender 命名空间发送的数据包会经过 `veth-recv` 的 XDP 层

**使用步骤：**

```bash
# 1. 创建虚拟网络环境
cd scripts
sudo ./network.sh setup

# 2. 在主命名空间运行 SketchLib 程序（挂载到 veth-recv）
cd ../build
sudo ./main

# 3. 在 sender 命名空间发送测试流量
# 使用 ping
sudo ip netns exec sender ping -c 100 10.10.10.2

# 使用 iperf3
sudo ip netns exec sender iperf3 -c 10.10.10.2 -t 10

# 使用 python-scapy 发送自定义流量
sudo ip netns exec sender python3 sender.py

# 4. 清理网络环境
cd ../scripts
sudo ./network.sh cleanup
```

**注意事项：**
- 所有操作需要 root 权限（eBPF 和网络命名空间操作需要）
- 如果没有物理网卡或不想影响真实网络，使用虚拟环境是最佳选择
- 虚拟环境的性能不如物理网卡，但足够用于功能测试和调试

## 🏗️ 架构设计

### 双 Buffer 机制

为了实现高效的数据交换和清空操作，SketchEBPFLib 在内核态使用了**双 Buffer 设计**：

```
内核态:                      用户态:
┌─────────────┐             ┌─────────────┐
│ counters_0  │◄────────────│ mmap[0]     │
└─────────────┘             └─────────────┘
       ▲                           │
       │ select_counter            │ swap()
       │ (ARRAY_OF_MAPS)           │
       ▼                           ▼
┌─────────────┐             ┌─────────────┐
│ counters_1  │◄────────────│ mmap[1]     │
└─────────────┘             └─────────────┘
```

**设计原因**：
- 内核态通过 `select_counter` (ARRAY_OF_MAPS) 选择当前活跃的 buffer
- 用户态调用 `swap()` 时，只需更新 `select_counter` 的指向（原子操作）
- 内核态立即切换到新 buffer，用户态可以安全地读取和清空旧 buffer
- **避免了锁竞争**：内核态持续写入，用户态可以并发清空，互不影响

### mmap 内存对齐

用户态通过 `mmap` 映射 eBPF map 时，必须使用 **stride 对齐**访问：

```c
#define MMAP_STRIDE(value_type) round_up(sizeof(value_type), 8)
```

**设计原因**：
- Linux 内核在实现 BPF_MAP_TYPE_ARRAY 时，强制将每个元素按 **8 字节对齐**存储
- 参考内核源码：[kernel/bpf/arraymap.c](https://github.com/torvalds/linux/blob/master/kernel/bpf/arraymap.c)
- 即使 `value_type` 是 4 字节的 `uint32_t`，在内存中也会占用 8 字节
- 用户态访问时必须按 stride 计算偏移，否则会读取到错误的数据

示例代码：
```cpp
// 错误：直接按数组访问
uint32_t value = counters_mmap[row * cols + col];

// 正确：按 stride 对齐访问
size_t offset = row * cols + col;
uint32_t* ptr = (uint32_t*)((char*)counters_mmap + offset * MMAP_STRIDE(uint32_t));
uint32_t value = *ptr;
```

### XDP 挂载点

eBPF 程序挂载在 **XDP (eXpress Data Path)** 层：
- XDP 是 Linux 内核中最早的数据包处理点，位于网卡驱动层
- 相比传统的 netfilter/iptables，XDP 具有更低的延迟和更高的吞吐量
- 支持 `XDP_PASS`（继续处理）、`XDP_DROP`（丢弃）、`XDP_TX`（转发）等动作
- SketchEBPFLib 使用 `XDP_PASS`，只统计流量而不影响数据包转发

## 🔍 性能分析

### 使用 bpftool 分析

```bash
# 查看加载的 eBPF 程序
sudo bpftool prog list

# 查看 map 信息
sudo bpftool map list

# 性能分析
sudo bpftool prog profile id <prog_id> duration 10 cycles

# 查看 map 内容
sudo bpftool map dump id <map_id>
```

## 🙏 致谢

使用的第三方库：
- **[BMv2 CRC32](https://github.com/p4lang/behavioral-model)** - BMv2 中的 CRC32 实现
- **[libbpf](https://github.com/libbpf/libbpf)** - eBPF 用户态库
- **[doctest](https://github.com/doctest/doctest)** - 轻量级 C++ 测试框架

感谢这些开源项目为社区做出的贡献！

## 📖 参考资料

- [eBPF 官方文档](https://ebpf.io/)
- [XDP Tutorial](https://github.com/xdp-project/xdp-tutorial)
- [libbpf-bootstrap](https://github.com/libbpf/libbpf-bootstrap)
