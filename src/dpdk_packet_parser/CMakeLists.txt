cmake_minimum_required(VERSION 3.16)

set(SRCS main.cpp)

find_package(SketchLib REQUIRED PATHS "${CMAKE_SOURCE_DIR}/lib/cmake/SketchLib")

# --- 共享链接版本 ---
add_executable(dpdk_packet_parser-shared ${SRCS})
target_link_libraries(dpdk_packet_parser-shared PRIVATE PkgConfig::DPDK SketchLib::SketchLib)
set_target_properties(dpdk_packet_parser-shared PROPERTIES OUTPUT_NAME "dpdk_packet_parser")