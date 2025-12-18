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

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/third_party)
include_directories(${AUTOGEN_DIR})

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(BPF_INCLUDES
    -I${CMAKE_CURRENT_SOURCE_DIR}/include
    -I${CMAKE_CURRENT_SOURCE_DIR}/third_party
    -I${AUTOGEN_DIR})

# 设置 eBPF 编译选项
set(BPF_CFLAGS -target bpf -O2 -g -Wall -Wextra)

# 编译通用依赖为 bpf.o
file(GLOB COMMON_SRCS "${SRC_DIR}/*.c")
set(COMMON_BPF_OBJS)
foreach(src ${COMMON_SRCS})
    get_filename_component(src_name ${src} NAME_WE)
    set(obj ${CMAKE_BINARY_DIR}/${src_name}.o)
    add_custom_command(
        OUTPUT ${obj}
        COMMAND ${CMAKE_C_COMPILER} ${BPF_CFLAGS} ${BPF_INCLUDES}
                -c ${src} -o ${obj}
        DEPENDS ${VMLINUX_H} ${src}
    )
    list(APPEND COMMON_BPF_OBJS ${obj})
endforeach()

# kernel 目录下的所有 bpf.c 生成 skeleton 放入 include/autogen
file(GLOB KERNEL_SRCS "${SRC_DIR}/kernel/*.bpf.c")
set(BPF_SKELETONS)
foreach(src ${KERNEL_SRCS})
    get_filename_component(kernel_name ${src} NAME_WE)
    string(REPLACE ".bpf" "" kernel_base ${kernel_name})

    set(kernel_obj ${CMAKE_BINARY_DIR}/${kernel_base}.bpf.o)
    set(combined_obj ${CMAKE_BINARY_DIR}/${kernel_base}.o)
    set(skeleton_hdr ${AUTOGEN_DIR}/${kernel_base}.skel.h)

    # 编译 src bpf
    add_custom_command(
        OUTPUT ${kernel_obj}
        COMMAND ${CMAKE_C_COMPILER} ${BPF_CFLAGS} ${BPF_INCLUDES}
                -c ${src} -o ${kernel_obj}
        DEPENDS ${VMLINUX_H} ${src} ${COMMON_SRCS} ${COMMON_BPF_OBJS}
    )

    # 链接通用依赖
    add_custom_command(
        OUTPUT ${combined_obj}
        COMMAND ${BPFTOOL} gen object ${combined_obj} ${COMMON_BPF_OBJS} ${kernel_obj}
        DEPENDS ${COMMON_BPF_OBJS} ${kernel_obj}
    )

    # 生成 skeleton 头文件
    add_custom_command(
        OUTPUT ${skeleton_hdr}
        COMMAND ${BPFTOOL} gen skeleton ${combined_obj} > ${skeleton_hdr}
        DEPENDS ${combined_obj}
    )

    add_custom_target(bpf_${kernel_base} DEPENDS ${skeleton_hdr})
    list(APPEND BPF_SKELETONS ${skeleton_hdr})
endforeach()

add_custom_target(skeleton ALL DEPENDS ${BPF_SKELETONS} vmlinux)

# 编译 user 目录下的用户态程序
file(GLOB USER_SRCS "${SRC_DIR}/user/*.cpp")

add_library(SketchEBPFLib STATIC ${USER_SRCS} ${SRC_DIR}/hash.c)
add_dependencies(SketchEBPFLib skeleton vmlinux)
target_include_directories(SketchEBPFLib PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${AUTOGEN_DIR}
        ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(SketchEBPFLib PUBLIC bpf)

enable_testing()
add_subdirectory(tests)