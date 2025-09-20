#include "../include/CachePolicy.h"
#include "../include/LruCache.h"
#include "LruCache.cpp"
#include "../include/FifoCache.h"
#include "FifoCache.cpp"
#include "../include/LruKCache.h"
#include "LruKCache.cpp"
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <chrono>
#include <thread>

/**
 * @brief 测试不同缓存策略的性能和行为差异
 */
void compareCacheStrategies() {
    std::cout << "=== Cache Strategy Comparison ===" << std::endl;
    
    const int capacity = 3;
    
    // 创建不同的缓存策略实例
    std::unique_ptr<CachePolicy<int, std::string>> lru = 
        std::make_unique<LruCache<int, std::string>>(capacity);
    
    std::unique_ptr<CachePolicy<int, std::string>> fifo = 
        std::make_unique<FifoCache<int, std::string>>(capacity);
    
    std::unique_ptr<CachePolicy<int, std::string>> lru2 = 
        std::make_unique<LruKCache<int, std::string>>(capacity, 2);
    
    std::unique_ptr<CachePolicy<int, std::string>> lru3 = 
        std::make_unique<LruKCache<int, std::string>>(capacity, 3);
    
    std::vector<std::pair<std::string, std::unique_ptr<CachePolicy<int, std::string>>&>> caches = {
        {"LRU", lru},
        {"FIFO", fifo},
        {"LRU-2", lru2},
        {"LRU-3", lru3}
    };
    
    // 测试数据和访问模式
    std::vector<std::pair<int, std::string>> initial_data = {
        {1, "One"}, {2, "Two"}, {3, "Three"}
    };
    
    std::vector<int> access_pattern = {1, 1, 2, 4, 5}; // 访问模式
    
    for (auto& [name, cache] : caches) {
        std::cout << "\\n--- " << name << " Strategy Test ---" << std::endl;
        
        // 初始化缓存
        for (const auto& [key, value] : initial_data) {
            cache->put(key, value);
            std::cout << "Insert (" << key << ", " << value << ")" << std::endl;
        }
        
        std::cout << "Initial state - Size: " << cache->size() << std::endl;
        
        // 执行访问模式
        for (int key : access_pattern) {
            if (key <= 3) {
                // 访问现有元素
                if (cache->contains(key)) {
                    std::string value = cache->get(key);
                    std::cout << "Access key " << key << ": " << value << std::endl;
                } else {
                    std::cout << "Access key " << key << ": Not found" << std::endl;
                }
            } else {
                // 插入新元素
                cache->put(key, "New" + std::to_string(key));
                std::cout << "Insert (" << key << ", New" << key << ")" << std::endl;
            }
            
            // 显示当前缓存状态
            std::cout << "  Current cache: ";
            for (int i = 1; i <= 5; ++i) {
                if (cache->contains(i)) {
                    std::cout << i << " ";
                }
            }
            std::cout << "(Size: " << cache->size() << ")" << std::endl;
        }
    }
}

/**
 * @brief 性能基准测试
 */
void performanceBenchmark() {
    std::cout << "\\n=== Performance Benchmark ===" << std::endl;
    
    const int capacity = 1000;
    const int operations = 10000;
    
    std::vector<std::pair<std::string, std::function<std::unique_ptr<CachePolicy<int, int>>()>>> factories = {
        {"LRU", [capacity]() { return std::make_unique<LruCache<int, int>>(capacity); }},
        {"FIFO", [capacity]() { return std::make_unique<FifoCache<int, int>>(capacity); }},
        {"LRU-2", [capacity]() { return std::make_unique<LruKCache<int, int>>(capacity, 2); }},
        {"LRU-3", [capacity]() { return std::make_unique<LruKCache<int, int>>(capacity, 3); }}
    };
    
    for (const auto& [name, factory] : factories) {
        auto cache = factory();
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // 执行大量操作
        for (int i = 0; i < operations; ++i) {
            int key = i % (capacity * 2); // 创建一些缓存命中和缺失
            
            if (i % 3 == 0) {
                // 插入操作
                cache->put(key, key * 2);
            } else {
                // 查询操作
                if (cache->contains(key)) {
                    cache->get(key);
                }
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        std::cout << name << " Strategy: " << operations << " operations took " 
                  << duration.count() << " microseconds" << std::endl;
    }
}

/**
 * @brief 演示LRU-K的优势场景
 */
void demonstrateLruKAdvantage() {
    std::cout << "\\n=== LRU-K Algorithm Advantage Demo ===" << std::endl;
    std::cout << "Scenario: Periodic access pattern + occasional access" << std::endl;
    
    const int capacity = 4;
    
    std::unique_ptr<CachePolicy<int, std::string>> lru = 
        std::make_unique<LruCache<int, std::string>>(capacity);
    
    std::unique_ptr<CachePolicy<int, std::string>> lru2 = 
        std::make_unique<LruKCache<int, std::string>>(capacity, 2);
    
    std::vector<std::pair<std::string, std::unique_ptr<CachePolicy<int, std::string>>&>> caches = {
        {"Traditional LRU", lru},
        {"LRU-2", lru2}
    };
    
    // 模拟周期性访问模式：1,2,3 是经常访问的，4是偶发访问的
    std::vector<std::pair<int, std::string>> data = {
        {1, "HotData1"}, {2, "HotData2"}, {3, "HotData3"}, {4, "ColdData"}
    };
    
    for (auto& [name, cache] : caches) {
        std::cout << "\\n--- " << name << " ---" << std::endl;
        
        // 初始化
        for (const auto& [key, value] : data) {
            cache->put(key, value);
        }
        
        // 模拟访问模式：多次访问1,2,3，偶尔访问4
        std::vector<int> access_sequence = {1, 2, 3, 1, 2, 3, 4, 1, 2, 3};
        
        for (int key : access_sequence) {
            if (cache->contains(key)) {
                cache->get(key);
                std::cout << "Access " << key << " hit";
            } else {
                std::cout << "Access " << key << " miss";
            }
            std::cout << " | Cache: ";
            for (int i = 1; i <= 4; ++i) {
                if (cache->contains(i)) std::cout << i << " ";
            }
            std::cout << std::endl;
        }
        
        // 插入新数据看淘汰效果
        std::cout << "Insert new data (5, NewData)..." << std::endl;
        cache->put(5, "NewData");
        std::cout << "Final cache: ";
        for (int i = 1; i <= 5; ++i) {
            if (cache->contains(i)) std::cout << i << " ";
        }
        std::cout << std::endl;
    }
    
    std::cout << "\\nAnalysis: LRU-2 better protects frequently accessed hot data," << std::endl;
    std::cout << "avoiding interference from occasional access." << std::endl;
}

int main() {
    try {
        compareCacheStrategies();
        performanceBenchmark();
        demonstrateLruKAdvantage();
        
        std::cout << "\\n🎉 Cache strategy comparison completed!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Test failed: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}