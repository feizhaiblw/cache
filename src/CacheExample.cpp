#include "../include/CachePolicy.h"
#include "../include/LruCache.h"
#include "LruCache.cpp"  // 包含实现文件，因为是模板类
#include "../include/FifoCache.h"
#include "FifoCache.cpp"  // 包含实现文件，因为是模板类
#include <iostream>
#include <memory>
#include <string>
#include <vector>

/**
 * @brief 演示如何使用不同的缓存策略
 */
void demonstrateCacheUsage() {
    // 创建不同类型的缓存策略
    std::unique_ptr<CachePolicy<int, std::string>> lru_cache = 
        std::make_unique<LruCache<int, std::string>>(3);
    
    std::unique_ptr<CachePolicy<int, std::string>> fifo_cache = 
        std::make_unique<FifoCache<int, std::string>>(3);
    
    // 测试数据
    std::vector<std::pair<int, std::string>> test_data = {
        {1, "One"}, {2, "Two"}, {3, "Three"}, {4, "Four"}, {5, "Five"}
    };
    
    // 测试LRU缓存
    std::cout << "=== 测试 " << lru_cache->getPolicyName() << " 缓存策略 ===" << std::endl;
    for (const auto& [key, value] : test_data) {
        lru_cache->put(key, value);
        std::cout << "插入: (" << key << ", " << value << "), 当前大小: " 
                  << lru_cache->size() << std::endl;
    }
    
    // 尝试获取一些值
    std::cout << "\n获取测试:" << std::endl;
    for (int key = 1; key <= 5; ++key) {
        if (lru_cache->contains(key)) {
            try {
                std::string value = lru_cache->get(key);
                std::cout << "键 " << key << " 的值: " << value << std::endl;
            } catch (const std::exception& e) {
                std::cout << "获取键 " << key << " 失败: " << e.what() << std::endl;
            }
        } else {
            std::cout << "键 " << key << " 不存在" << std::endl;
        }
    }
    
    std::cout << "\n" << std::string(50, '=') << std::endl;
    
    // 测试FIFO缓存
    std::cout << "=== 测试 " << fifo_cache->getPolicyName() << " 缓存策略 ===" << std::endl;
    for (const auto& [key, value] : test_data) {
        fifo_cache->put(key, value);
        std::cout << "插入: (" << key << ", " << value << "), 当前大小: " 
                  << fifo_cache->size() << std::endl;
    }
    
    // 尝试获取一些值
    std::cout << "\n获取测试:" << std::endl;
    for (int key = 1; key <= 5; ++key) {
        if (fifo_cache->contains(key)) {
            try {
                std::string value = fifo_cache->get(key);
                std::cout << "键 " << key << " 的值: " << value << std::endl;
            } catch (const std::exception& e) {
                std::cout << "获取键 " << key << " 失败: " << e.what() << std::endl;
            }
        } else {
            std::cout << "键 " << key << " 不存在" << std::endl;
        }
    }
}

/**
 * @brief 通用的缓存测试函数
 * 
 * @param cache 要测试的缓存策略
 * @param strategy_name 策略名称
 */
void testCacheStrategy(std::unique_ptr<CachePolicy<int, std::string>>& cache, 
                      const std::string& strategy_name) {
    std::cout << "\n=== 测试 " << strategy_name << " 策略 ===" << std::endl;
    
    // 测试基本操作
    cache->put(1, "First");
    cache->put(2, "Second");
    cache->put(3, "Third");
    
    std::cout << "容量: " << cache->capacity() << std::endl;
    std::cout << "当前大小: " << cache->size() << std::endl;
    std::cout << "是否为空: " << (cache->empty() ? "是" : "否") << std::endl;
    
    // 清空测试
    cache->clear();
    std::cout << "清空后大小: " << cache->size() << std::endl;
}

int main() {
    try {
        demonstrateCacheUsage();
        
        // 创建缓存实例进行额外测试
        std::unique_ptr<CachePolicy<int, std::string>> lru = 
            std::make_unique<LruCache<int, std::string>>(2);
        std::unique_ptr<CachePolicy<int, std::string>> fifo = 
            std::make_unique<FifoCache<int, std::string>>(2);
        
        testCacheStrategy(lru, "LRU");
        testCacheStrategy(fifo, "FIFO");
        
    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}