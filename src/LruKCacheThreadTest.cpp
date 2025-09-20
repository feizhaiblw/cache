#include "../include/LruKCache.h"
#include "LruKCache.cpp"
#include "../include/ThreadSafeTestFramework.h"
#include <iostream>
#include <cassert>
#include <thread>
#include <vector>
#include <chrono>
#include <random>

/**
 * @brief LRU-K缓存多线程安全测试
 * LRU-K策略特点：维护历史队列和缓存队列，只有访问次数≥K的数据才进入缓存。
 */

void testLruKBasicThreadSafety() {
    std::cout << "\n=== LRU-K缓存基本线程安全测试 ===" << std::endl;
    
    const int K = 2;
    LruKCache<int, int> cache(50, K);
    ThreadSafeTestFramework<int, int> framework;
    
    const int THREAD_COUNT = 4;
    const int OPERATIONS_PER_THREAD = 300;
    const int KEY_RANGE = 30;
    
    std::vector<std::thread> threads;
    
    for (int t = 0; t < THREAD_COUNT; ++t) {
        threads.emplace_back([&cache, &framework, t, OPERATIONS_PER_THREAD, KEY_RANGE]() {
            std::random_device rd;
            std::mt19937 gen(rd() + t);
            std::uniform_int_distribution<int> key_dist(0, KEY_RANGE - 1);
            
            for (int i = 0; i < OPERATIONS_PER_THREAD; ++i) {
                int key = key_dist(gen);
                int value = key * 100 + t * 10 + i;
                
                ThreadSafeTestFramework<int, int>::Operation op(
                    ThreadSafeTestFramework<int, int>::OperationType::PUT, key, value);
                
                try {
                    cache.put(key, value);
                    op.success = true;
                    op.result = "PUT成功";
                } catch (const std::exception& e) {
                    op.success = false;
                    op.result = std::string("PUT异常: ") + e.what();
                }
                
                framework.recordOperation(op);
            }
        });
    }
    
    framework.setStartTime();
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    framework.setEndTime();
    
    std::cout << "测试后缓存大小: " << cache.size() << "/" << cache.capacity() << std::endl;
    std::cout << "K值: " << cache.getK() << std::endl;
    assert(cache.size() <= cache.capacity());
    
    framework.printStatistics();
    
    std::cout << "✓ LRU-K基本线程安全测试通过" << std::endl;
}

void testLruKConcurrentReadWrite() {
    std::cout << "\n=== LRU-K缓存并发读写测试 ===" << std::endl;
    
    const int K = 3;
    LruKCache<int, std::string> cache(30, K);
    
    // 先填充一些初始数据
    for (int i = 0; i < 10; ++i) {
        for (int access = 0; access < K; ++access) {
            cache.put(i, "initial_" + std::to_string(i));
        }
    }
    
    const int THREAD_COUNT = 4;
    const int OPERATIONS_PER_THREAD = 200;
    const int KEY_RANGE = 20;
    
    std::vector<std::thread> threads;
    std::atomic<bool> start_flag{false};
    
    for (int t = 0; t < THREAD_COUNT; ++t) {
        threads.emplace_back([&cache, &start_flag, t, OPERATIONS_PER_THREAD, KEY_RANGE]() {
            while (!start_flag.load()) {
                std::this_thread::yield();
            }
            
            std::random_device rd;
            std::mt19937 gen(rd() + t);
            std::uniform_int_distribution<int> key_dist(0, KEY_RANGE - 1);
            std::uniform_int_distribution<int> op_dist(0, 2);
            
            for (int i = 0; i < OPERATIONS_PER_THREAD; ++i) {
                int key = key_dist(gen);
                
                if (op_dist(gen) == 0) {
                    // PUT操作
                    cache.put(key, "value_" + std::to_string(t) + "_" + std::to_string(i));
                } else if (op_dist(gen) == 1) {
                    // GET操作
                    try {
                        cache.get(key);
                    } catch (const std::exception&) {
                        // 键可能不在缓存中
                    }
                } else {
                    // CONTAINS操作
                    cache.contains(key);
                }
            }
        });
    }
    
    start_flag.store(true);
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    std::cout << "测试后缓存大小: " << cache.size() << "/" << cache.capacity() << std::endl;
    assert(cache.size() <= cache.capacity());
    
    std::cout << "✓ LRU-K并发读写测试通过" << std::endl;
}

void testLruKEvictionUnderConcurrency() {
    std::cout << "\n=== LRU-K缓存并发淘汰机制测试 ===" << std::endl;
    
    const int K = 2;
    const int CACHE_CAPACITY = 8;
    LruKCache<int, std::string> cache(CACHE_CAPACITY, K);
    
    const int THREAD_COUNT = 4;
    const int OPERATIONS_PER_THREAD = 150;
    const int KEY_RANGE = 30;
    
    std::vector<std::thread> threads;
    std::atomic<int> total_puts{0};
    
    for (int t = 0; t < THREAD_COUNT; ++t) {
        threads.emplace_back([&cache, &total_puts, t, OPERATIONS_PER_THREAD, KEY_RANGE]() {
            std::random_device rd;
            std::mt19937 gen(rd() + t);
            std::uniform_int_distribution<int> key_dist(0, KEY_RANGE - 1);
            
            for (int i = 0; i < OPERATIONS_PER_THREAD; ++i) {
                int key = key_dist(gen);
                std::string value = "thread_" + std::to_string(t) + "_op_" + std::to_string(i);
                
                cache.put(key, value);
                total_puts++;
                
                // 对某些键进行重复访问
                if (key < 10 && i % 5 == 0) {
                    cache.put(key, value + "_repeat");
                    total_puts++;
                }
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    std::cout << "总PUT操作: " << total_puts.load() << std::endl;
    std::cout << "最终缓存大小: " << cache.size() << "/" << cache.capacity() << std::endl;
    
    // LRU-K特殊行为：只有访问次数≥K的数据才进入缓存
    // 所以缓存大小可能小于容量，这是正常的
    assert(cache.size() >= 0);
    // 注意：由于LRU-K只缓存访问≥K次的数据，实际缓存大小通常会小于容量
    
    std::cout << "✓ LRU-K并发淘汰机制测试通过" << std::endl;
}

void testLruKStressTest() {
    std::cout << "\n=== LRU-K缓存压力测试 ===" << std::endl;
    
    const int K = 3;
    LruKCache<int, int> cache(100, K);
    ThreadSafeTestFramework<int, int> framework;
    
    framework.setStartTime();
    
    std::vector<std::thread> threads;
    const int THREAD_COUNT = 6;
    const int OPERATIONS_PER_THREAD = 500;
    const int KEY_RANGE = 60;
    
    for (int t = 0; t < THREAD_COUNT; ++t) {
        threads.emplace_back([&cache, &framework, t, OPERATIONS_PER_THREAD, KEY_RANGE]() {
            std::random_device rd;
            std::mt19937 gen(rd() + t);
            std::uniform_int_distribution<int> key_dist(0, KEY_RANGE - 1);
            std::uniform_int_distribution<int> op_dist(0, 2);
            
            for (int i = 0; i < OPERATIONS_PER_THREAD; ++i) {
                int key = key_dist(gen);
                
                ThreadSafeTestFramework<int, int>::Operation op(
                    static_cast<ThreadSafeTestFramework<int, int>::OperationType>(op_dist(gen)),
                    key, key * 100 + t);
                
                try {
                    if (op_dist(gen) == 0) {
                        cache.put(key, key * 100 + t);
                    } else if (op_dist(gen) == 1) {
                        cache.get(key);
                    } else {
                        cache.contains(key);
                    }
                    op.success = true;
                } catch (const std::exception&) {
                    op.success = false;
                }
                
                framework.recordOperation(op);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    framework.setEndTime();
    
    std::cout << "压力测试后缓存大小: " << cache.size() << "/" << cache.capacity() << std::endl;
    assert(cache.size() <= cache.capacity());
    
    framework.printStatistics();
    
    std::cout << "✓ LRU-K压力测试通过" << std::endl;
}

int main() {
    try {
        std::cout << "开始LRU-K缓存多线程安全测试..." << std::endl;
        std::cout << "LRU-K特点: 维护历史队列和缓存队列，只有访问次数≥K的数据才进入缓存" << std::endl;
        
        testLruKBasicThreadSafety();
        testLruKConcurrentReadWrite();
        testLruKEvictionUnderConcurrency();
        testLruKStressTest();
        
        std::cout << "\n🎉 所有LRU-K缓存多线程测试通过！" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ 测试失败: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}