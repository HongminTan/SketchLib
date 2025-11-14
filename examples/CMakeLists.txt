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
# 1. 确保 SketchLib 已经编译完成
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

# ============================================================================
# 创建示例可执行文件
# ============================================================================

# 定义所有示例程序
set(EXAMPLE_SOURCES
    example_countmin.cpp        # cm
    example_countsketch.cpp     # cs
    example_elasticsketch.cpp   # es
    example_hashpipe.cpp        # hp
    example_univmon.cpp         # um
    example_sketchlearn.cpp     # sl
    example_flowradar.cpp       # fr
    example_mvsketch.cpp        # mv
)

# 为每个示例创建可执行文件
foreach(EXAMPLE_SRC ${EXAMPLE_SOURCES})
    # 获取文件名
    get_filename_component(EXAMPLE_NAME ${EXAMPLE_SRC} NAME_WE)
    
    # 创建可执行文件
    add_executable(${EXAMPLE_NAME} ${EXAMPLE_SRC})
    
    # 链接 SketchLib 库
    target_link_libraries(${EXAMPLE_NAME} PRIVATE SketchLib)
    
    # 包含头文件目录
    target_include_directories(${EXAMPLE_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/third_party
    )
    
    # 编译器设置
    if(WIN32)
        target_compile_definitions(${EXAMPLE_NAME} PRIVATE WIN32_LEAN_AND_MEAN)
        if(MSVC)
            target_compile_options(${EXAMPLE_NAME} PRIVATE /W3)
        endif()
        if(MINGW)
            target_compile_options(${EXAMPLE_NAME} PRIVATE -Wall -Wextra)
        endif()
    else()
        target_compile_options(${EXAMPLE_NAME} PRIVATE -Wall -Wextra -O2)
    endif()
    
    # 安装规则
    install(TARGETS ${EXAMPLE_NAME} DESTINATION bin/examples)
endforeach()

# ============================================================================
# 调试信息
# ============================================================================
#
# 取消下面的注释来查看路径信息
# message(STATUS "Example source file: ${CMAKE_CURRENT_SOURCE_DIR}/example.cpp")
# message(STATUS "SketchLib include dir: ${CMAKE_SOURCE_DIR}/include")
# message(STATUS "Third party dir: ${CMAKE_SOURCE_DIR}/third_party")
#
# ============================================================================