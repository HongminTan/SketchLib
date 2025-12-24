cmake_minimum_required(VERSION 3.16)

project(SketchLib-dpdk LANGUAGES C CXX)

set(CMAKE_C_COMPILER "clang")
set(CMAKE_CXX_COMPILER "clang++")
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig REQUIRED QUIET)

pkg_check_modules(DPDK REQUIRED IMPORTED_TARGET libdpdk)

add_compile_definitions(ALLOW_EXPERIMENTAL_API)

# 子目录
add_subdirectory(src)