#include "../include/LruCache.h"
#include "LruCache.cpp"
#include "../include/ThreadSafeTestFramework.h"
#include <iostream>
#include <cassert>
#include <thread>
#include <vector>
#include <chrono>
#include <random>
#include <mutex>
#include <shared_mutex>

/**
 * @brief LRU缓存多线程安全测试
 * 
 * 根据项目规范，LRU缓存默认不是线程安全的，需要外部同步措施。
 * 本测试验证在使用适当同步机制的情况下，LRU缓存在多线程环境下的正确性。
 */

void testBasicThreadSafety() {
    std::cout << "\n=== LRU缓存基本线程安全测试 ===" << std::endl;
    
    LruCache<int, int> cache(100);
    ThreadSafeTestFramework<int, int> framework;
    std::shared_mutex cache_mutex;
    
    const int THREAD_COUNT = 4;
    const int OPERATIONS_PER_THREAD = 500;
    const int KEY_RANGE = 50;
    
    std::vector<std::thread> threads;
    
    // 启动多个线程同时进行PUT操作
    for (int t = 0; t < THREAD_COUNT; ++t) {
        threads.emplace_back([&cache, &cache_mutex, &framework, t, OPERATIONS_PER_THREAD, KEY_RANGE]() {
            std::random_device rd;
            std::mt19937 gen(rd() + t);
            std::uniform_int_distribution<int> key_dist(0, KEY_RANGE - 1);
            
            for (int i = 0; i < OPERATIONS_PER_THREAD; ++i) {
                int key = key_dist(gen);
                int value = key * 100 + t * 10 + i; // 保证唯一性
                
                ThreadSafeTestFramework<int, int>::Operation op(
                    ThreadSafeTestFramework<int, int>::OperationType::PUT, key, value);
                
                try {
                    std::unique_lock<std::shared_mutex> lock(cache_mutex);
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
    
    // 验证缓存状态
    {
        std::shared_lock<std::shared_mutex> lock(cache_mutex);
        std::cout << "测试后缓存大小: " << cache.size() << "/" << cache.capacity() << std::endl;
        assert(cache.size() <= cache.capacity());
        assert(cache.size() >= 0);
    }
    
    framework.printStatistics();
    
    // 数据一致性验证
    bool consistent = framework.validateDataConsistency(cache, KEY_RANGE);
    assert(consistent);
    
    std::cout << "✓ 基本线程安全测试通过" << std::endl;
}

void testConcurrentReadWrite() {
    std::cout << "\n=== LRU缓存并发读写测试 ===" << std::endl;
    
    LruCache<int, std::string> cache(50);
    ThreadSafeTestFramework<int, std::string> framework;
    std::shared_mutex cache_mutex;
    
    const int WRITER_COUNT = 2;
    const int READER_COUNT = 4;
    const int OPERATIONS_PER_THREAD = 300;
    const int KEY_RANGE = 30;
    
    // 先填充一些初始数据
    {
        std::unique_lock<std::shared_mutex> lock(cache_mutex);
        for (int i = 0; i < 20; ++i) {
            cache.put(i, "initial_" + std::to_string(i));
        }
    }
    
    std::vector<std::thread> threads;
    std::atomic<bool> start_flag{false};
    
    // 创建写线程
    for (int t = 0; t < WRITER_COUNT; ++t) {
        threads.emplace_back([&cache, &cache_mutex, &framework, &start_flag, t, OPERATIONS_PER_THREAD, KEY_RANGE]() {
            // 等待开始信号
            while (!start_flag.load()) {
                std::this_thread::yield();
            }
            
            std::random_device rd;
            std::mt19937 gen(rd() + t);
            std::uniform_int_distribution<int> key_dist(0, KEY_RANGE - 1);
            
            for (int i = 0; i < OPERATIONS_PER_THREAD; ++i) {
                int key = key_dist(gen);
                std::string value = "writer_" + std::to_string(t) + "_" + std::to_string(i);
                
                ThreadSafeTestFramework<int, std::string>::Operation op(
                    ThreadSafeTestFramework<int, std::string>::OperationType::PUT, key, value);
                
                try {
                    std::unique_lock<std::shared_mutex> lock(cache_mutex);
                    cache.put(key, value);
                    op.success = true;
                    op.result = "WRITE成功";
                } catch (const std::exception& e) {
                    op.success = false;
                    op.result = std::string("WRITE异常: ") + e.what();
                }
                
                framework.recordOperation(op);
                
                // 随机暂停，增加竞争
                if (i % 10 == 0) {
                    std::this_thread::sleep_for(std::chrono::microseconds(10));
                }
            }
        });
    }
    
    // 创建读线程
    for (int t = 0; t < READER_COUNT; ++t) {
        threads.emplace_back([&cache, &cache_mutex, &framework, &start_flag, t, OPERATIONS_PER_THREAD, KEY_RANGE]() {
            // 等待开始信号
            while (!start_flag.load()) {
                std::this_thread::yield();
            }
            
            std::random_device rd;
            std::mt19937 gen(rd() + t + 100);
            std::uniform_int_distribution<int> key_dist(0, KEY_RANGE - 1);
            std::uniform_int_distribution<int> op_dist(0, 1); // 0=GET, 1=CONTAINS
            
            for (int i = 0; i < OPERATIONS_PER_THREAD; ++i) {
                int key = key_dist(gen);
                
                if (op_dist(gen) == 0) {
                    // GET操作
                    ThreadSafeTestFramework<int, std::string>::Operation op(
                        ThreadSafeTestFramework<int, std::string>::OperationType::GET, key, "");
                    
                    try {
                        std::shared_lock<std::shared_mutex> lock(cache_mutex);
                        std::string value = cache.get(key);
                        op.success = true;
                        op.result = "READ成功: " + value;
                    } catch (const std::exception& e) {
                        op.success = false;
                        op.result = std::string("READ异常: ") + e.what();
                    }
                    
                    framework.recordOperation(op);
                } else {
                    // CONTAINS操作
                    ThreadSafeTestFramework<int, std::string>::Operation op(
                        ThreadSafeTestFramework<int, std::string>::OperationType::CONTAINS, key, "");
                    
                    try {
                        std::shared_lock<std::shared_mutex> lock(cache_mutex);
                        bool exists = cache.contains(key);
                        op.success = true;
                        op.result = exists ? "CONTAINS:存在" : "CONTAINS:不存在";
                    } catch (const std::exception& e) {
                        op.success = false;
                        op.result = std::string("CONTAINS异常: ") + e.what();
                    }
                    
                    framework.recordOperation(op);
                }
            }
        });
    }
    
    framework.setStartTime();
    start_flag.store(true); // 开始测试
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    framework.setEndTime();
    
    // 验证缓存状态
    {
        std::shared_lock<std::shared_mutex> lock(cache_mutex);
        std::cout << "测试后缓存大小: " << cache.size() << "/" << cache.capacity() << std::endl;
        assert(cache.size() <= cache.capacity());
    }
    
    framework.printStatistics();
    
    // 验证数据一致性
    bool consistent = framework.validateDataConsistency(cache, KEY_RANGE);
    assert(consistent);
    
    std::cout << "✓ 并发读写测试通过" << std::endl;
}

void testLruEvictionUnderConcurrency() {
    std::cout << "\n=== LRU缓存并发淘汰机制测试 ===" << std::endl;
    
    const int CACHE_CAPACITY = 10;
    LruCache<int, std::string> cache(CACHE_CAPACITY);
    std::shared_mutex cache_mutex;
    
    const int THREAD_COUNT = 3;
    const int OPERATIONS_PER_THREAD = 200;
    const int KEY_RANGE = 50; // 远大于缓存容量，确保会发生淘汰
    
    std::vector<std::thread> threads;
    std::atomic<int> total_puts{0};
    
    // 多线程并发插入，测试LRU淘汰机制是否正常工作
    for (int t = 0; t < THREAD_COUNT; ++t) {
        threads.emplace_back([&cache, &cache_mutex, &total_puts, t, OPERATIONS_PER_THREAD, KEY_RANGE]() {
            std::random_device rd;
            std::mt19937 gen(rd() + t);
            std::uniform_int_distribution<int> key_dist(0, KEY_RANGE - 1);
            
            for (int i = 0; i < OPERATIONS_PER_THREAD; ++i) {
                int key = key_dist(gen);
                std::string value = "thread_" + std::to_string(t) + "_op_" + std::to_string(i);
                
                {
                    std::unique_lock<std::shared_mutex> lock(cache_mutex);
                    cache.put(key, value);
                    total_puts++;
                }
                
                // 偶尔进行读取操作，影响LRU顺序
                if (i % 10 == 0) {
                    int read_key = key_dist(gen);
                    std::shared_lock<std::shared_mutex> lock(cache_mutex);
                    try {
                        cache.get(read_key);
                    } catch (const std::exception&) {
                        // 键不存在是正常的
                    }
                }
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // 验证缓存大小不超过容量
    {
        std::shared_lock<std::shared_mutex> lock(cache_mutex);
        std::cout << "总共执行PUT操作: " << total_puts.load() << std::endl;
        std::cout << "最终缓存大小: " << cache.size() << "/" << cache.capacity() << std::endl;
        
        assert(cache.size() <= CACHE_CAPACITY);
        assert(cache.size() >= 0);
        
        // 由于大量插入，缓存应该接近满容量
        assert(cache.size() == CACHE_CAPACITY);
    }
    
    std::cout << "✓ 并发淘汰机制测试通过" << std::endl;
}

void testConcurrentClearOperations() {
    std::cout << "\n=== LRU缓存并发清空测试 ===" << std::endl;
    
    LruCache<int, int> cache(100);
    std::shared_mutex cache_mutex;
    
    // 先填充缓存
    {
        std::unique_lock<std::shared_mutex> lock(cache_mutex);
        for (int i = 0; i < 50; ++i) {
            cache.put(i, i * 10);
        }
    }
    
    const int THREAD_COUNT = 4;
    std::vector<std::thread> threads;
    std::atomic<bool> start_flag{false};
    std::atomic<int> clear_count{0};
    std::atomic<int> operation_count{0};
    
    // 创建测试线程：一些线程进行正常操作，一些线程进行清空操作
    for (int t = 0; t < THREAD_COUNT; ++t) {
        threads.emplace_back([&cache, &cache_mutex, &start_flag, &clear_count, &operation_count, t]() {
            while (!start_flag.load()) {
                std::this_thread::yield();
            }
            
            std::random_device rd;
            std::mt19937 gen(rd() + t);
            
            for (int i = 0; i < 100; ++i) {
                if (t == 0 && i % 20 == 0) {
                    // 线程0负责偶尔清空缓存
                    std::unique_lock<std::shared_mutex> lock(cache_mutex);
                    cache.clear();
                    clear_count++;
                    std::cout << "线程" << t << "执行清空操作，次数: " << clear_count.load() << std::endl;
                } else {
                    // 其他线程进行正常的PUT/GET操作
                    std::uniform_int_distribution<int> key_dist(0, 20);
                    int key = key_dist(gen);
                    
                    if (i % 2 == 0) {
                        // PUT操作
                        std::unique_lock<std::shared_mutex> lock(cache_mutex);
                        cache.put(key, key * 100 + t);
                    } else {
                        // GET操作
                        std::shared_lock<std::shared_mutex> lock(cache_mutex);
                        try {
                            cache.get(key);
                        } catch (const std::exception&) {
                            // 正常情况，缓存可能被清空了
                        }
                    }
                    operation_count++;
                }
                
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        });
    }
    
    start_flag.store(true);
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    std::cout << "清空操作次数: " << clear_count.load() << std::endl;
    std::cout << "其他操作次数: " << operation_count.load() << std::endl;
    
    // 验证最终状态
    {
        std::shared_lock<std::shared_mutex> lock(cache_mutex);
        std::cout << "最终缓存大小: " << cache.size() << std::endl;
        assert(cache.size() >= 0);
        assert(cache.size() <= cache.capacity());
    }
    
    std::cout << "✓ 并发清空测试通过" << std::endl;
}

void testStressTest() {
    std::cout << "\n=== LRU缓存压力测试 ===" << std::endl;
    
    LruCache<int, int> cache(200);
    ThreadSafeTestFramework<int, int> framework;
    
    // 高强度并发测试
    framework.mixedOperationsTest(cache, 8, 1000, 100);
    
    framework.printStatistics();
    
    // 验证缓存状态
    std::shared_mutex cache_mutex;
    {
        std::shared_lock<std::shared_mutex> lock(cache_mutex);
        std::cout << "压力测试后缓存大小: " << cache.size() << "/" << cache.capacity() << std::endl;
        assert(cache.size() <= cache.capacity());
        assert(cache.size() >= 0);
    }
    
    std::cout << "✓ 压力测试通过" << std::endl;
}

void testExceptionsUnderConcurrency() {
    std::cout << "\n=== LRU缓存并发异常处理测试 ===" << std::endl;
    
    LruCache<int, int> cache(10);
    std::shared_mutex cache_mutex;
    
    const int THREAD_COUNT = 4;
    const int OPERATIONS_PER_THREAD = 100;
    
    std::vector<std::thread> threads;
    std::atomic<int> exception_count{0};
    
    for (int t = 0; t < THREAD_COUNT; ++t) {
        threads.emplace_back([&cache, &cache_mutex, &exception_count, t, OPERATIONS_PER_THREAD]() {
            std::random_device rd;
            std::mt19937 gen(rd() + t);
            std::uniform_int_distribution<int> key_dist(100, 200); // 使用不存在的键
            
            for (int i = 0; i < OPERATIONS_PER_THREAD; ++i) {
                int key = key_dist(gen);
                
                try {
                    std::shared_lock<std::shared_mutex> lock(cache_mutex);
                    cache.get(key); // 尝试获取不存在的键
                } catch (const std::out_of_range&) {
                    exception_count++;
                    // 这是预期的异常
                } catch (const std::exception& e) {
                    std::cout << "意外异常: " << e.what() << std::endl;
                }
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    std::cout << "捕获异常次数: " << exception_count.load() << std::endl;
    assert(exception_count.load() > 0); // 应该有异常被捕获
    
    std::cout << "✓ 并发异常处理测试通过" << std::endl;
}

int main() {
    try {
        std::cout << "开始LRU缓存多线程安全测试..." << std::endl;
        std::cout << "注意: 根据项目规范，缓存策略默认不是线程安全的，本测试使用外部同步措施" << std::endl;
        
        testBasicThreadSafety();
        testConcurrentReadWrite();
        testLruEvictionUnderConcurrency();
        testConcurrentClearOperations();
        testStressTest();
        testExceptionsUnderConcurrency();
        
        std::cout << "\n🎉 所有LRU缓存多线程测试通过！" << std::endl;
        std::cout << "验证了使用适当外部同步措施时，LRU缓存在多线程环境下的正确性。" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ 测试失败: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}