cmake_minimum_required(VERSION 3.16)

set(SRCS main.c)

# --- 共享链接版本 ---
add_executable(countmin-shared ${SRCS})
target_link_libraries(countmin-shared PRIVATE PkgConfig::DPDK)
set_target_properties(countmin-shared PROPERTIES OUTPUT_NAME "countmin")