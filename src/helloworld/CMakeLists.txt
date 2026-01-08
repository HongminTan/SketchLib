cmake_minimum_required(VERSION 3.16)

set(SRCS main.cpp)

# --- 共享链接版本 ---
add_executable(helloworld-shared ${SRCS})
target_link_libraries(helloworld-shared PRIVATE PkgConfig::DPDK)
set_target_properties(helloworld-shared PROPERTIES OUTPUT_NAME "helloworld")