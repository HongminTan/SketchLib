# ============================================================================
# SketchLib 示例程序 CMake 配置
# ============================================================================
#
# 本文件用于编译 SketchLib 的示例程序
#
# ============================================================================
# 重要：路径配置说明
# ============================================================================
#
# 如果你的项目结构与默认不同，需要修改以下路径：
#
# 1. ${CMAKE_SOURCE_DIR}/include
#    这是 SketchLib 头文件的位置
#    - 如果你的头文件在其他位置，修改为实际路径
#    - 例如：/path/to/your/SketchLib/include
#
# 2. ${CMAKE_SOURCE_DIR}/third_party
#    这是第三方库（MurmurHash3, SpookyV2）的位置
#    - 如果第三方库在其他位置，修改为实际路径
#    - 例如：/path/to/your/SketchLib/third_party
#
# 3. target_link_libraries 中的 SketchLib
#    这是链接的库名称
#    - 如果你的库名称不同，修改为实际名称
#    - 如果使用静态库文件，可以直接指定路径：
#      target_link_libraries(example PRIVATE /path/to/libSketchLib.a)
#
# ============================================================================
# 使用方法
# ============================================================================
#
# 1. 确保 SketchLib 已经编译完成（生成了 libSketchLib.a 或 SketchLib.lib）
#
# 2. 配置并编译示例：
#    cd build
#    cmake -DBUILD_EXAMPLES=ON ..
#    cmake --build .
#
# 3. 运行示例：
#    Windows: .\examples\example.exe
#    Linux:   ./examples/example
#
# ============================================================================

# 创建示例可执行文件
add_executable(example example.cpp)

# ============================================================================
# 链接 SketchLib 库
# ============================================================================
#
# 方式1：使用库目标名称（推荐，如果 SketchLib 在同一个 CMake 项目中）
target_link_libraries(example PRIVATE SketchLib)
#
# 方式2：使用库文件路径（如果 SketchLib 是外部编译好的库）
# target_link_libraries(example PRIVATE
#     ${CMAKE_SOURCE_DIR}/build/libSketchLib.a  # Linux/macOS
#     # 或
#     ${CMAKE_SOURCE_DIR}/build/SketchLib.lib   # Windows MSVC
# )
#
# ============================================================================

# ============================================================================
# 包含头文件目录
# ============================================================================
#
# 方式1：使用 CMAKE_SOURCE_DIR（推荐）
target_include_directories(example PRIVATE
    ${CMAKE_SOURCE_DIR}/include      # SketchLib 头文件
    ${CMAKE_SOURCE_DIR}/third_party  # 第三方库头文件
)
#
# 方式2：使用相对路径
# target_include_directories(example PRIVATE
#     ${CMAKE_CURRENT_SOURCE_DIR}/../include
#     ${CMAKE_CURRENT_SOURCE_DIR}/../third_party
# )
#
# 方式3：使用绝对路径（如果 SketchLib 在固定位置）
# target_include_directories(example PRIVATE
#     /absolute/path/to/SketchLib/include
#     /absolute/path/to/SketchLib/third_party
# )
#
# ============================================================================

# ============================================================================
# 编译器特定设置
# ============================================================================

if(WIN32)
    # Windows 平台设置
    target_compile_definitions(example PRIVATE WIN32_LEAN_AND_MEAN)

    # 如果使用 MSVC 编译器
    if(MSVC)
        target_compile_options(example PRIVATE /W3)
    endif()

    # 如果使用 MinGW
    if(MINGW)
        target_compile_options(example PRIVATE -Wall -Wextra)
    endif()
else()
    # Linux/macOS 平台设置
    target_compile_options(example PRIVATE -Wall -Wextra -O2)
endif()

# ============================================================================
# 安装规则（可选）
# ============================================================================
#
# 将示例程序安装到指定目录
install(TARGETS example DESTINATION bin/examples)
#
# ============================================================================

# ============================================================================
# 调试信息（可选，用于检查路径是否正确）
# ============================================================================
#
# 取消下面的注释来查看路径信息
# message(STATUS "Example source file: ${CMAKE_CURRENT_SOURCE_DIR}/example.cpp")
# message(STATUS "SketchLib include dir: ${CMAKE_SOURCE_DIR}/include")
# message(STATUS "Third party dir: ${CMAKE_SOURCE_DIR}/third_party")
#
# ============================================================================