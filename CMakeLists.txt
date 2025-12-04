cmake_minimum_required(VERSION 3.16)
project(SketchEBPFLib LANGUAGES C CXX)

set(CMAKE_C_COMPILER "clang")
set(CMAKE_CXX_COMPILER "clang++")
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置编译器 Flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -O2")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")

# 检查 bpftool 工具
find_program(BPFTOOL bpftool REQUIRED)

# 生成 vmlinux.h 到 include/autogen
set(AUTOGEN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include/autogen")
set(VMLINUX_H "${AUTOGEN_DIR}/vmlinux.h")

# 创建 autogen 目录
file(MAKE_DIRECTORY ${AUTOGEN_DIR})

add_custom_command(
    OUTPUT ${VMLINUX_H}
    COMMAND ${BPFTOOL} btf dump file /sys/kernel/btf/vmlinux format c > ${VMLINUX_H}
    VERBATIM
)

# 生成 vmlinux.h
add_custom_target(vmlinux DEPENDS ${VMLINUX_H})
