cmake_minimum_required(VERSION 3.10)
project(CacheProject LANGUAGES CXX)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置编译器选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# 查找线程库
find_package(Threads REQUIRED)

# 包含目录
include_directories(include)

# 收集所有缓存实现源文件（排除测试和示例文件）
file(GLOB CACHE_IMPL_SOURCES 
    "src/LruCache.cpp"
    "src/FifoCache.cpp" 
    "src/LfuCache.cpp"
    "src/LruKCache.cpp"
)

# 创建缓存策略静态库
add_library(CacheLib STATIC ${CACHE_IMPL_SOURCES})
target_include_directories(CacheLib PUBLIC include)
target_link_libraries(CacheLib Threads::Threads)

# ========== 基础功能测试可执行文件 ==========
# LRU缓存测试
add_executable(LruCacheTest src/LruCacheTest.cpp)
target_link_libraries(LruCacheTest CacheLib Threads::Threads)

# FIFO缓存测试
add_executable(FifoCacheTest src/FifoCacheTest.cpp)
target_link_libraries(FifoCacheTest CacheLib Threads::Threads)

# LFU缓存测试
add_executable(LfuCacheTest src/LfuCacheTest.cpp)
target_link_libraries(LfuCacheTest CacheLib Threads::Threads)

# LRU-K缓存测试
add_executable(LruKCacheTest src/LruKCacheTest.cpp)
target_link_libraries(LruKCacheTest CacheLib Threads::Threads)

# ========== 示例程序可执行文件 ==========
# 缓存使用示例
add_executable(CacheExample src/CacheExample.cpp)
target_link_libraries(CacheExample CacheLib Threads::Threads)

# 缓存性能对比示例
add_executable(CacheComparison src/CacheComparison.cpp)
target_link_libraries(CacheComparison CacheLib Threads::Threads)

# ========== 自定义目标和测试 ==========
# 启用CTest
enable_testing()

# 添加基础功能测试
add_test(NAME LruCacheTest COMMAND LruCacheTest)
add_test(NAME FifoCacheTest COMMAND FifoCacheTest)
add_test(NAME LfuCacheTest COMMAND LfuCacheTest)
add_test(NAME LruKCacheTest COMMAND LruKCacheTest)

# 添加多线程测试
add_test(NAME LruCacheThreadTest COMMAND LruCacheThreadTest)
add_test(NAME FifoCacheThreadTest COMMAND FifoCacheThreadTest)
add_test(NAME LfuCacheThreadTest COMMAND LfuCacheThreadTest)
add_test(NAME LruKCacheThreadTest COMMAND LruKCacheThreadTest)

# 创建便捷目标
add_custom_target(tests
    DEPENDS LruCacheTest FifoCacheTest LfuCacheTest LruKCacheTest
    COMMENT "Building all basic tests"
)

add_custom_target(thread_tests
    DEPENDS LruCacheThreadTest FifoCacheThreadTest LfuCacheThreadTest LruKCacheThreadTest
    COMMENT "Building all thread safety tests"
)

add_custom_target(examples
    DEPENDS CacheExample CacheComparison
    COMMENT "Building all example programs"
)

add_custom_target(run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --verbose
    DEPENDS tests
    COMMENT "Running all basic tests"
)

add_custom_target(run_thread_tests
    COMMAND ${CMAKE_SOURCE_DIR}/run_multithread_tests.sh
    DEPENDS thread_tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running multi-thread safety tests"
)

# ========== 多线程安全测试可执行文件 ==========
# LRU缓存多线程测试
add_executable(LruCacheThreadTest src/LruCacheThreadTest.cpp)
target_link_libraries(LruCacheThreadTest CacheLib Threads::Threads)

# FIFO缓存多线程测试
add_executable(FifoCacheThreadTest src/FifoCacheThreadTest.cpp)
target_link_libraries(FifoCacheThreadTest CacheLib Threads::Threads)

# LFU缓存多线程测试
add_executable(LfuCacheThreadTest src/LfuCacheThreadTest.cpp)
target_link_libraries(LfuCacheThreadTest CacheLib Threads::Threads)

# LRU-K缓存多线程测试
add_executable(LruKCacheThreadTest src/LruKCacheThreadTest.cpp)
target_link_libraries(LruKCacheThreadTest CacheLib Threads::Threads)