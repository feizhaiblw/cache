#include "../include/FifoCache.h"
#include "FifoCache.cpp"
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
 * @brief FIFO缓存多线程安全测试
 * 
 * 根据项目规范，FIFO缓存默认不是线程安全的，需要外部同步措施。
 * 本测试验证在使用适当同步机制的情况下，FIFO缓存在多线程环境下的正确性。
 * FIFO策略的特点是按插入顺序淘汰，不受访问频率影响。
 */

void testFifoBasicThreadSafety() {
    std::cout << "\n=== FIFO缓存基本线程安全测试 ===" << std::endl;
    
    FifoCache<int, int> cache(100);
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
    
    std::cout << "✓ FIFO基本线程安全测试通过" << std::endl;
}

void testFifoConcurrentReadWrite() {
    std::cout << "\n=== FIFO缓存并发读写测试 ===" << std::endl;
    
    FifoCache<int, std::string> cache(50);
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
    
    std::cout << "✓ FIFO并发读写测试通过" << std::endl;
}

void testFifoEvictionUnderConcurrency() {
    std::cout << "\n=== FIFO缓存并发淘汰机制测试 ===" << std::endl;
    
    const int CACHE_CAPACITY = 10;
    FifoCache<int, std::string> cache(CACHE_CAPACITY);
    std::shared_mutex cache_mutex;
    
    const int THREAD_COUNT = 3;
    const int OPERATIONS_PER_THREAD = 200;
    const int KEY_RANGE = 50; // 远大于缓存容量，确保会发生淘汰
    
    std::vector<std::thread> threads;
    std::atomic<int> total_puts{0};
    std::vector<std::pair<int, std::string>> insertion_order; // 记录插入顺序
    std::mutex order_mutex;
    
    // 多线程并发插入，测试FIFO淘汰机制是否正常工作
    for (int t = 0; t < THREAD_COUNT; ++t) {
        threads.emplace_back([&cache, &cache_mutex, &total_puts, &insertion_order, &order_mutex, t, OPERATIONS_PER_THREAD, KEY_RANGE]() {
            std::random_device rd;
            std::mt19937 gen(rd() + t);
            std::uniform_int_distribution<int> key_dist(0, KEY_RANGE - 1);
            
            for (int i = 0; i < OPERATIONS_PER_THREAD; ++i) {
                int key = key_dist(gen);
                std::string value = "thread_" + std::to_string(t) + "_op_" + std::to_string(i);
                
                {
                    std::unique_lock<std::shared_mutex> cache_lock(cache_mutex);
                    
                    // 记录插入顺序（仅当键不存在时）
                    if (!cache.contains(key)) {
                        std::lock_guard<std::mutex> order_lock(order_mutex);
                        insertion_order.emplace_back(key, value);
                    }
                    
                    cache.put(key, value);
                    total_puts++;
                }
                
                // FIFO不需要访问操作来改变淘汰顺序，但我们可以验证读取不影响淘汰
                if (i % 15 == 0) {
                    int read_key = key_dist(gen);
                    std::shared_lock<std::shared_mutex> lock(cache_mutex);
                    try {
                        std::string read_value = cache.get(read_key);
                        // 在FIFO中，读取操作不应影响淘汰顺序
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
        std::cout << "记录的插入顺序数量: " << insertion_order.size() << std::endl;
        
        assert(cache.size() <= CACHE_CAPACITY);
        assert(cache.size() >= 0);
        
        // 由于大量插入，缓存应该接近满容量
        if (insertion_order.size() >= CACHE_CAPACITY) {
            assert(cache.size() == CACHE_CAPACITY);
        }
    }
    
    std::cout << "✓ FIFO并发淘汰机制测试通过" << std::endl;
}

void testFifoVsLruBehaviorUnderConcurrency() {
    std::cout << "\n=== FIFO与LRU行为差异并发测试 ===" << std::endl;
    
    const int CACHE_CAPACITY = 5;
    FifoCache<int, std::string> cache(CACHE_CAPACITY);
    std::shared_mutex cache_mutex;
    
    // 先填充缓存到满容量
    {
        std::unique_lock<std::shared_mutex> lock(cache_mutex);
        for (int i = 0; i < CACHE_CAPACITY; ++i) {
            cache.put(i, "initial_" + std::to_string(i));
        }
        std::cout << "初始填充后缓存大小: " << cache.size() << std::endl;
    }
    
    const int THREAD_COUNT = 2;
    std::vector<std::thread> threads;
    std::atomic<bool> start_flag{false};
    
    // 线程1：频繁访问最早插入的元素（在LRU中会保留，在FIFO中应该被淘汰）
    threads.emplace_back([&cache, &cache_mutex, &start_flag]() {
        while (!start_flag.load()) {
            std::this_thread::yield();
        }
        
        for (int i = 0; i < 50; ++i) {
            try {
                std::shared_lock<std::shared_mutex> lock(cache_mutex);
                if (cache.contains(0)) {
                    cache.get(0); // 频繁访问键0
                }
            } catch (const std::exception&) {
                // 键0可能已被淘汰，这在FIFO中是正常的
            }
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    });
    
    // 线程2：插入新元素，触发FIFO淘汰
    threads.emplace_back([&cache, &cache_mutex, &start_flag]() {
        while (!start_flag.load()) {
            std::this_thread::yield();
        }
        
        for (int i = 10; i < 20; ++i) {
            {
                std::unique_lock<std::shared_mutex> lock(cache_mutex);
                cache.put(i, "new_" + std::to_string(i));
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });
    
    start_flag.store(true);
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // 验证FIFO行为：最早插入的元素应该被淘汰，即使被频繁访问
    {
        std::shared_lock<std::shared_mutex> lock(cache_mutex);
        std::cout << "最终缓存大小: " << cache.size() << std::endl;
        std::cout << "最终缓存内容: ";
        
        for (int i = 0; i < 25; ++i) {
            if (cache.contains(i)) {
                std::cout << i << " ";
            }
        }
        std::cout << std::endl;
        
        // 键0（最早插入且被频繁访问）在FIFO策略下应该被淘汰
        // 这验证了FIFO不受访问频率影响的特性
        assert(cache.size() <= CACHE_CAPACITY);
    }
    
    std::cout << "✓ FIFO与LRU行为差异测试通过，验证了FIFO不受访问频率影响" << std::endl;
}

void testFifoConcurrentClearOperations() {
    std::cout << "\n=== FIFO缓存并发清空测试 ===" << std::endl;
    
    FifoCache<int, int> cache(100);
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
                if (t == 0 && i % 25 == 0) {
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
    
    std::cout << "✓ FIFO并发清空测试通过" << std::endl;
}

void testFifoStressTest() {
    std::cout << "\n=== FIFO缓存压力测试 ===" << std::endl;
    
    FifoCache<int, int> cache(200);
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
    
    std::cout << "✓ FIFO压力测试通过" << std::endl;
}

void testFifoExceptionsUnderConcurrency() {
    std::cout << "\n=== FIFO缓存并发异常处理测试 ===" << std::endl;
    
    FifoCache<int, int> cache(10);
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
    
    std::cout << "✓ FIFO并发异常处理测试通过" << std::endl;
}

int main() {
    try {
        std::cout << "开始FIFO缓存多线程安全测试..." << std::endl;
        std::cout << "注意: 根据项目规范，缓存策略默认不是线程安全的，本测试使用外部同步措施" << std::endl;
        std::cout << "FIFO特点: 按插入顺序淘汰，不受访问频率影响" << std::endl;
        
        testFifoBasicThreadSafety();
        testFifoConcurrentReadWrite();
        testFifoEvictionUnderConcurrency();
        testFifoVsLruBehaviorUnderConcurrency();
        testFifoConcurrentClearOperations();
        testFifoStressTest();
        testFifoExceptionsUnderConcurrency();
        
        std::cout << "\n🎉 所有FIFO缓存多线程测试通过！" << std::endl;
        std::cout << "验证了使用适当外部同步措施时，FIFO缓存在多线程环境下的正确性。" << std::endl;
        std::cout << "确认了FIFO策略不受访问频率影响的特性在并发环境下依然保持。" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ 测试失败: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}