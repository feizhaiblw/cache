# Makefile for Cache Project
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -Iinclude
LDFLAGS = -lpthread

# 源文件目录
SRC_DIR = src
INC_DIR = include
BUILD_DIR = build

# 基础测试目标
TEST_TARGETS = lru_test fifo_test lfu_test lruk_test
# 多线程测试目标
THREAD_TEST_TARGETS = lru_thread_test fifo_thread_test lfu_thread_test lruk_thread_test
# 示例程序目标
EXAMPLE_TARGETS = cache_example cache_comparison

# 所有目标
TARGETS = $(TEST_TARGETS) $(THREAD_TEST_TARGETS) $(EXAMPLE_TARGETS)

# 创建构建目录
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# 默认目标
all: $(BUILD_DIR) $(TARGETS)

# ========== 基础测试目标 ==========
# LRU缓存测试
lru_test: $(BUILD_DIR) $(SRC_DIR)/LruCacheTest.cpp $(INC_DIR)/LruCache.h $(INC_DIR)/CachePolicy.h $(SRC_DIR)/LruCache.cpp
	$(CXX) $(CXXFLAGS) $(SRC_DIR)/LruCacheTest.cpp -o $(BUILD_DIR)/lru_test

# FIFO缓存测试
fifo_test: $(BUILD_DIR) $(SRC_DIR)/FifoCacheTest.cpp $(INC_DIR)/FifoCache.h $(INC_DIR)/CachePolicy.h $(SRC_DIR)/FifoCache.cpp
	$(CXX) $(CXXFLAGS) $(SRC_DIR)/FifoCacheTest.cpp -o $(BUILD_DIR)/fifo_test

# LFU缓存测试
lfu_test: $(BUILD_DIR) $(SRC_DIR)/LfuCacheTest.cpp $(INC_DIR)/LfuCache.h $(INC_DIR)/CachePolicy.h $(SRC_DIR)/LfuCache.cpp
	$(CXX) $(CXXFLAGS) $(SRC_DIR)/LfuCacheTest.cpp -o $(BUILD_DIR)/lfu_test

# LRU-K缓存测试
lruk_test: $(BUILD_DIR) $(SRC_DIR)/LruKCacheTest.cpp $(INC_DIR)/LruKCache.h $(INC_DIR)/CachePolicy.h $(SRC_DIR)/LruKCache.cpp
	$(CXX) $(CXXFLAGS) $(SRC_DIR)/LruKCacheTest.cpp -o $(BUILD_DIR)/lruk_test

# ========== 多线程测试目标 ==========
# LRU缓存多线程测试
lru_thread_test: $(BUILD_DIR) $(SRC_DIR)/LruCacheThreadTest.cpp $(INC_DIR)/LruCache.h $(INC_DIR)/ThreadSafeTestFramework.h $(SRC_DIR)/LruCache.cpp
	$(CXX) $(CXXFLAGS) $(SRC_DIR)/LruCacheThreadTest.cpp -o $(BUILD_DIR)/LruCacheThreadTest $(LDFLAGS)

# FIFO缓存多线程测试
fifo_thread_test: $(BUILD_DIR) $(SRC_DIR)/FifoCacheThreadTest.cpp $(INC_DIR)/FifoCache.h $(INC_DIR)/ThreadSafeTestFramework.h $(SRC_DIR)/FifoCache.cpp
	$(CXX) $(CXXFLAGS) $(SRC_DIR)/FifoCacheThreadTest.cpp -o $(BUILD_DIR)/FifoCacheThreadTest $(LDFLAGS)

# LFU缓存多线程测试
lfu_thread_test: $(BUILD_DIR) $(SRC_DIR)/LfuCacheThreadTest.cpp $(INC_DIR)/LfuCache.h $(INC_DIR)/ThreadSafeTestFramework.h $(SRC_DIR)/LfuCache.cpp
	$(CXX) $(CXXFLAGS) $(SRC_DIR)/LfuCacheThreadTest.cpp -o $(BUILD_DIR)/LfuCacheThreadTest $(LDFLAGS)

# LRU-K缓存多线程测试
lruk_thread_test: $(BUILD_DIR) $(SRC_DIR)/LruKCacheThreadTest.cpp $(INC_DIR)/LruKCache.h $(INC_DIR)/ThreadSafeTestFramework.h $(SRC_DIR)/LruKCache.cpp
	$(CXX) $(CXXFLAGS) $(SRC_DIR)/LruKCacheThreadTest.cpp -o $(BUILD_DIR)/LruKCacheThreadTest $(LDFLAGS)

# ========== 示例程序目标 ==========
# 缓存示例程序
cache_example: $(BUILD_DIR) $(SRC_DIR)/CacheExample.cpp $(INC_DIR)/LruCache.h $(INC_DIR)/FifoCache.h $(INC_DIR)/CachePolicy.h $(SRC_DIR)/LruCache.cpp $(SRC_DIR)/FifoCache.cpp
	$(CXX) $(CXXFLAGS) $(SRC_DIR)/CacheExample.cpp -o $(BUILD_DIR)/cache_example

# 缓存对比程序
cache_comparison: $(BUILD_DIR) $(SRC_DIR)/CacheComparison.cpp $(INC_DIR)/LruCache.h $(INC_DIR)/FifoCache.h $(INC_DIR)/LfuCache.h $(INC_DIR)/LruKCache.h $(INC_DIR)/CachePolicy.h $(SRC_DIR)/LruCache.cpp $(SRC_DIR)/FifoCache.cpp $(SRC_DIR)/LfuCache.cpp $(SRC_DIR)/LruKCache.cpp
	$(CXX) $(CXXFLAGS) $(SRC_DIR)/CacheComparison.cpp -o $(BUILD_DIR)/cache_comparison

# 清理编译文件
clean:
	rm -rf $(BUILD_DIR) $(TARGETS)

# ========== 运行目标 ==========
# 运行所有基础测试
test: $(TEST_TARGETS)
	@echo "=== 运行所有基础测试 ==="
	$(BUILD_DIR)/lru_test
	$(BUILD_DIR)/fifo_test
	$(BUILD_DIR)/lfu_test
	$(BUILD_DIR)/lruk_test
	@echo "=== 所有基础测试完成 ==="

# 运行多线程测试
thread_test: $(THREAD_TEST_TARGETS)
	@echo "=== 运行多线程测试脚本 ==="
	./run_multithread_tests.sh

# 运行示例程序
demo: cache_example
	@echo "=== 运行缓存示例程序 ==="
	$(BUILD_DIR)/cache_example

# 运行缓存对比程序
comparison: cache_comparison
	@echo "=== 运行缓存对比程序 ==="
	$(BUILD_DIR)/cache_comparison

# ========== 便捷目标 ==========
# 编译所有基础测试
tests: $(TEST_TARGETS)

# 编译所有多线程测试
thread_tests: $(THREAD_TEST_TARGETS)

# 编译所有示例程序
examples: $(EXAMPLE_TARGETS)

# 显示帮助信息
help:
	@echo "=== 缓存策略框架构建系统 ==="
	@echo ""
	@echo "编译目标:"
	@echo "  all              - 编译所有目标"
	@echo "  tests            - 编译所有基础测试"
	@echo "  thread_tests     - 编译所有多线程测试"
	@echo "  examples         - 编译所有示例程序"
	@echo ""
	@echo "单独编译目标:"
	@echo "  lru_test         - 编译LRU缓存测试"
	@echo "  fifo_test        - 编译FIFO缓存测试"
	@echo "  lfu_test         - 编译LFU缓存测试"
	@echo "  lruk_test        - 编译LRU-K缓存测试"
	@echo "  lru_thread_test  - 编译LRU多线程测试"
	@echo "  fifo_thread_test - 编译FIFO多线程测试"
	@echo "  lfu_thread_test  - 编译LFU多线程测试"
	@echo "  lruk_thread_test - 编译LRU-K多线程测试"
	@echo "  cache_example    - 编译缓存示例程序"
	@echo "  cache_comparison - 编译缓存对比程序"
	@echo ""
	@echo "运行目标:"
	@echo "  test             - 运行所有基础测试"
	@echo "  thread_test      - 运行多线程测试"
	@echo "  demo             - 运行缓存示例"
	@echo "  comparison       - 运行缓存对比"
	@echo ""
	@echo "工具目标:"
	@echo "  clean            - 清理编译文件"
	@echo "  help             - 显示此帮助信息"
	@echo ""
	@echo "项目结构:"
	@echo "  include/         - 头文件目录"
	@echo "  src/             - 源文件目录"
	@echo "  build/           - 编译输出目录"
	@echo ""
	@echo "支持的缓存策略:"
	@echo "  • LRU    - 最近最少使用策略"
	@echo "  • FIFO   - 先进先出策略"
	@echo "  • LFU    - 最少使用频率策略"
	@echo "  • LRU-K  - LRU的K距离变种"

.PHONY: all clean test thread_test demo comparison tests thread_tests examples help