# 测试配置
find_package(Threads REQUIRED)

# 测试可执行文件
file (GLOB TEST_SOURCES "*.cpp")

add_executable(sketch_tests ${TEST_SOURCES})

target_link_libraries(sketch_tests
    PRIVATE
        SketchLib
        Threads::Threads
)

target_include_directories(sketch_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

# 编译器特定设置
if(WIN32)
    target_compile_definitions(sketch_tests PRIVATE WIN32_LEAN_AND_MEAN)
endif()

# 添加测试
enable_testing()
add_test(NAME SketchLibTests COMMAND sketch_tests)