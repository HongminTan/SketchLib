# 测试配置
find_package(Threads REQUIRED)

# 测试可执行文件
file (GLOB TEST_SOURCES "*.cpp")

add_executable(sketch_tests ${TEST_SOURCES})

target_link_libraries(sketch_tests
    PRIVATE
        SketchEBPFLib
        Threads::Threads
)

target_include_directories(sketch_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../third_party
)

# 添加测试
enable_testing()
add_test(NAME SketchLibTests COMMAND sketch_tests)