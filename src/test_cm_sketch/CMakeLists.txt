cmake_minimum_required(VERSION 3.16)

set(SRCS main.cpp)

find_package(SketchLib REQUIRED PATHS "${CMAKE_SOURCE_DIR}/lib/cmake/SketchLib")

# --- 共享链接版本 ---
add_executable(test_cm_sketch-shared ${SRCS})
target_link_libraries(test_cm_sketch-shared PRIVATE PkgConfig::DPDK SketchLib::SketchLib)
set_target_properties(test_cm_sketch-shared PROPERTIES OUTPUT_NAME "test_cm_sketch")