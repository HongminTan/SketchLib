cmake_minimum_required(VERSION 3.16)
project(SketchLib VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置编译器 Flags
if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3 /wd26819")
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2 -Wno-implicit-fallthrough")
endif()

# 头文件目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/third_party)

# 收集源文件
file(GLOB_RECURSE SKETCH_SOURCES
    "src/*.cpp"
    "src/*.c"
    "third_party/*.cpp"
    "third_party/*.c"
)

file(GLOB_RECURSE SKETCH_HEADERS
    "include/*.h"
    "include/*.hpp"
    "third_party/*.h"
    "third_party/*.hpp"
)

# 创建静态库
add_library(SketchLib STATIC ${SKETCH_SOURCES} ${SKETCH_HEADERS})

# 设置库的属性
set_target_properties(SketchLib PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER "${SKETCH_HEADERS}")

# 包含目录设置
target_include_directories(SketchLib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# 编译器特定设置
if(WIN32)
    target_compile_definitions(SketchLib PRIVATE WIN32_LEAN_AND_MEAN)
endif()

# 安装设置
include(GNUInstallDirs)

install(TARGETS SketchLib
    EXPORT SketchLibTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# 安装头文件
install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)

# 导出配置
install(EXPORT SketchLibTargets
    FILE SketchLibTargets.cmake
    NAMESPACE SketchLib::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SketchLib
)

# 创建配置文件
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    SketchLibConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/SketchLibConfig.cmake.in
    SketchLibConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SketchLib
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/SketchLibConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/SketchLibConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SketchLib
)

# 构建测试
option(BUILD_TESTS "Build tests" ON)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# 构建示例
option(BUILD_EXAMPLES "Build examples" ON)
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()