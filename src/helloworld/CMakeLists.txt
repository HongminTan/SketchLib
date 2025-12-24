cmake_minimum_required(VERSION 3.16)

set(SRCS main.cpp)

# --- 共享链接版本 ---
add_executable(${PROJECT_NAME}-shared ${SRCS})
target_link_libraries(${PROJECT_NAME}-shared PRIVATE PkgConfig::DPDK)
set_target_properties(${PROJECT_NAME}-shared PROPERTIES OUTPUT_NAME "${PROJECT_NAME}")