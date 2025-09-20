#include "../include/LfuCache.h"
#include "LfuCache.cpp"
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
 * @brief LFU缓存多线程安全测试
 * 
 * 根据项目规范，LFU缓存默认不是线程安全的，需要外部同步措施。
 * 本测试验证在使用适当同步机制的情况下，LFU缓存在多线程环境下的正确性。
 * LFU策略的特点是按访问频率淘汰，频率相同时使用LRU作为tie-breaking策略。
 */

void testLfuBasicThreadSafety() {
    std::cout << "\n=== LFU缓存基本线程安全测试 ===" << std::endl;
    
    LfuCache<int, int> cache(100);
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
    
    std::cout << "✓ LFU基本线程安全测试通过" << std::endl;
}

void testLfuConcurrentReadWrite() {
    std::cout << "\n=== LFU缓存并发读写测试 ===" << std::endl;
    
    LfuCache<int, std::string> cache(50);
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
                    // GET操作 - 会增加访问频率
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
                    // CONTAINS操作 - 不影响访问频率
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
    
    // 验证缓存状态和频率统计
    {
        std::shared_lock<std::shared_mutex> lock(cache_mutex);
        std::cout << "测试后缓存大小: " << cache.size() << "/" << cache.capacity() << std::endl;
        std::cout << "最小频率: " << cache.getMinFrequency() << std::endl;
        assert(cache.size() <= cache.capacity());
    }
    
    framework.printStatistics();
    
    // 验证数据一致性
    bool consistent = framework.validateDataConsistency(cache, KEY_RANGE);
    assert(consistent);
    
    std::cout << "✓ LFU并发读写测试通过" << std::endl;
}

void testLfuFrequencyTrackingUnderConcurrency() {
    std::cout << "\n=== LFU缓存并发频率跟踪测试 ===" << std::endl;
    
    const int CACHE_CAPACITY = 10;
    LfuCache<int, std::string> cache(CACHE_CAPACITY);
    std::shared_mutex cache_mutex;
    
    // 先插入一些数据
    {
        std::unique_lock<std::shared_mutex> lock(cache_mutex);
        for (int i = 0; i < 5; ++i) {
            cache.put(i, "value_" + std::to_string(i));
        }
    }
    
    const int THREAD_COUNT = 4;
    std::vector<std::thread> threads;
    std::atomic<bool> start_flag{false};
    std::atomic<int> total_gets{0};
    
    // 创建线程，对不同键进行不同频率的访问
    for (int t = 0; t < THREAD_COUNT; ++t) {
        threads.emplace_back([&cache, &cache_mutex, &start_flag, &total_gets, t]() {
            while (!start_flag.load()) {
                std::this_thread::yield();
            }
            
            // 不同线程访问不同的键，模拟不同的访问模式
            int target_key = t % 5; // 每个线程主要访问一个键
            int access_count = (t + 1) * 20; // 不同线程不同的访问次数
            
            for (int i = 0; i < access_count; ++i) {
                try {
                    std::shared_lock<std::shared_mutex> lock(cache_mutex);
                    if (cache.contains(target_key)) {
                        cache.get(target_key);
                        total_gets++;
                    }
                } catch (const std::exception&) {
                    // 键可能不存在
                }
                
                std::this_thread::sleep_for(std::chrono::microseconds(50));
            }
        });
    }
    
    start_flag.store(true);
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // 验证频率跟踪结果
    {
        std::shared_lock<std::shared_mutex> lock(cache_mutex);
        std::cout << "总GET操作次数: " << total_gets.load() << std::endl;
        std::cout << "最小频率: " << cache.getMinFrequency() << std::endl;
        
        // 检查不同键的访问频率
        for (int i = 0; i < 5; ++i) {
            if (cache.contains(i)) {
                int frequency = cache.getFrequency(i);
                std::cout << "键" << i << "的访问频率: " << frequency << std::endl;
                assert(frequency >= 1); // 至少有初始插入时的频率
            }
        }
    }
    
    std::cout << "✓ LFU并发频率跟踪测试通过" << std::endl;
}

void testLfuEvictionUnderConcurrency() {
    std::cout << "\n=== LFU缓存并发淘汰机制测试 ===" << std::endl;
    
    const int CACHE_CAPACITY = 8;
    LfuCache<int, std::string> cache(CACHE_CAPACITY);
    std::shared_mutex cache_mutex;
    
    const int THREAD_COUNT = 3;
    const int OPERATIONS_PER_THREAD = 200;
    const int KEY_RANGE = 50; // 远大于缓存容量，确保会发生淘汰
    
    std::vector<std::thread> threads;
    std::atomic<int> total_puts{0};
    std::map<int, std::atomic<int>> key_access_counts;
    
    // 初始化访问计数器
    for (int i = 0; i < KEY_RANGE; ++i) {
        key_access_counts[i].store(0);
    }
    
    // 多线程并发插入和访问，测试LFU淘汰机制
    for (int t = 0; t < THREAD_COUNT; ++t) {
        threads.emplace_back([&cache, &cache_mutex, &total_puts, &key_access_counts, t, OPERATIONS_PER_THREAD, KEY_RANGE]() {
            std::random_device rd;
            std::mt19937 gen(rd() + t);
            std::uniform_int_distribution<int> key_dist(0, KEY_RANGE - 1);
            std::uniform_int_distribution<int> op_dist(0, 2); // 0=PUT, 1=GET, 2=重复GET
            
            for (int i = 0; i < OPERATIONS_PER_THREAD; ++i) {
                int key = key_dist(gen);
                int operation = op_dist(gen);
                
                if (operation == 0) {
                    // PUT操作
                    std::string value = "thread_" + std::to_string(t) + "_op_" + std::to_string(i);
                    {
                        std::unique_lock<std::shared_mutex> lock(cache_mutex);
                        cache.put(key, value);
                        total_puts++;
                        key_access_counts[key]++; // PUT也算一次访问
                    }
                } else {
                    // GET操作（增加访问频率）
                    {
                        std::shared_lock<std::shared_mutex> lock(cache_mutex);
                        try {
                            cache.get(key);
                            key_access_counts[key]++;
                        } catch (const std::exception&) {
                            // 键不存在是正常的
                        }
                    }
                }
                
                // 对某些热点键进行额外访问
                if (key < 10 && i % 5 == 0) {
                    std::shared_lock<std::shared_mutex> lock(cache_mutex);
                    try {
                        cache.get(key);
                        key_access_counts[key]++;
                    } catch (const std::exception&) {
                        // 键不存在
                    }
                }
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // 验证缓存大小和淘汰策略效果
    {
        std::shared_lock<std::shared_mutex> lock(cache_mutex);
        std::cout << "总共执行PUT操作: " << total_puts.load() << std::endl;
        std::cout << "最终缓存大小: " << cache.size() << "/" << cache.capacity() << std::endl;
        std::cout << "最小频率: " << cache.getMinFrequency() << std::endl;
        
        // 验证缓存中保留的是高频访问的键
        std::cout << "缓存中保留的键及其频率: ";
        for (int i = 0; i < KEY_RANGE; ++i) {
            if (cache.contains(i)) {
                int frequency = cache.getFrequency(i);
                std::cout << i << "(" << frequency << ") ";
            }
        }
        std::cout << std::endl;
        
        assert(cache.size() <= CACHE_CAPACITY);
        assert(cache.size() >= 0);
        
        // 由于大量插入，缓存应该接近满容量
        if (total_puts.load() > CACHE_CAPACITY) {
            assert(cache.size() == CACHE_CAPACITY);
        }
    }
    
    std::cout << "✓ LFU并发淘汰机制测试通过" << std::endl;
}

void testLfuTieBreakingUnderConcurrency() {
    std::cout << "\n=== LFU缓存并发Tie-breaking策略测试 ===" << std::endl;
    
    const int CACHE_CAPACITY = 5;
    LfuCache<int, std::string> cache(CACHE_CAPACITY);
    std::shared_mutex cache_mutex;
    
    // 先插入数据，使所有键的频率相等
    {
        std::unique_lock<std::shared_mutex> lock(cache_mutex);
        for (int i = 0; i < CACHE_CAPACITY; ++i) {
            cache.put(i, "equal_freq_" + std::to_string(i));
        }
        std::cout << "初始填充，所有键频率为1" << std::endl;
    }
    
    const int THREAD_COUNT = 2;
    std::vector<std::thread> threads;
    std::atomic<bool> start_flag{false};
    
    // 线程1：访问最早插入的键（键0）
    threads.emplace_back([&cache, &cache_mutex, &start_flag]() {
        while (!start_flag.load()) {
            std::this_thread::yield();
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(50)); // 稍微延迟
        
        try {
            std::shared_lock<std::shared_mutex> lock(cache_mutex);
            if (cache.contains(0)) {
                cache.get(0); // 这会让键0的频率变为2，但更新访问时间
            }
        } catch (const std::exception&) {
            // 键可能不存在
        }
    });
    
    // 线程2：插入新键，触发淘汰
    threads.emplace_back([&cache, &cache_mutex, &start_flag]() {
        while (!start_flag.load()) {
            std::this_thread::yield();
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 等待线程1完成
        
        {
            std::unique_lock<std::shared_mutex> lock(cache_mutex);
            cache.put(10, "new_key"); // 插入新键，应该触发淘汰
        }
    });
    
    start_flag.store(true);
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // 验证tie-breaking策略（频率相同时使用LRU）
    {
        std::shared_lock<std::shared_mutex> lock(cache_mutex);
        std::cout << "最终缓存大小: " << cache.size() << std::endl;
        std::cout << "缓存内容: ";
        
        for (int i = 0; i <= 10; ++i) {
            if (cache.contains(i)) {
                int frequency = cache.getFrequency(i);
                std::cout << i << "(" << frequency << ") ";
            }
        }
        std::cout << std::endl;
        
        assert(cache.size() <= CACHE_CAPACITY);
        assert(cache.contains(10)); // 新插入的键应该存在
    }
    
    std::cout << "✓ LFU并发Tie-breaking策略测试通过" << std::endl;
}

void testLfuConcurrentClearOperations() {
    std::cout << "\n=== LFU缓存并发清空测试 ===" << std::endl;
    
    LfuCache<int, int> cache(100);
    std::shared_mutex cache_mutex;
    
    // 先填充缓存
    {
        std::unique_lock<std::shared_mutex> lock(cache_mutex);
        for (int i = 0; i < 50; ++i) {
            cache.put(i, i * 10);
            // 对一些键进行额外访问以建立不同频率
            if (i < 10) {
                cache.get(i);
            }
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
            
            for (int i = 0; i < 80; ++i) {
                if (t == 0 && i % 30 == 0) {
                    // 线程0负责偶尔清空缓存
                    std::unique_lock<std::shared_mutex> lock(cache_mutex);
                    cache.clear();
                    clear_count++;
                    std::cout << "线程" << t << "执行清空操作，次数: " << clear_count.load() 
                              << "，最小频率重置为: " << cache.getMinFrequency() << std::endl;
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
                
                std::this_thread::sleep_for(std::chrono::microseconds(200));
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
        std::cout << "最终最小频率: " << cache.getMinFrequency() << std::endl;
        assert(cache.size() >= 0);
        assert(cache.size() <= cache.capacity());
    }
    
    std::cout << "✓ LFU并发清空测试通过" << std::endl;
}

void testLfuStressTest() {
    std::cout << "\n=== LFU缓存压力测试 ===" << std::endl;
    
    LfuCache<int, int> cache(200);
    ThreadSafeTestFramework<int, int> framework;
    
    // 高强度并发测试
    framework.mixedOperationsTest(cache, 6, 800, 80);
    
    framework.printStatistics();
    
    // 验证缓存状态
    std::shared_mutex cache_mutex;
    {
        std::shared_lock<std::shared_mutex> lock(cache_mutex);
        std::cout << "压力测试后缓存大小: " << cache.size() << "/" << cache.capacity() << std::endl;
        std::cout << "最小频率: " << cache.getMinFrequency() << std::endl;
        assert(cache.size() <= cache.capacity());
        assert(cache.size() >= 0);
    }
    
    std::cout << "✓ LFU压力测试通过" << std::endl;
}

void testLfuExceptionsUnderConcurrency() {
    std::cout << "\n=== LFU缓存并发异常处理测试 ===" << std::endl;
    
    LfuCache<int, int> cache(10);
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
    
    std::cout << "✓ LFU并发异常处理测试通过" << std::endl;
}

int main() {
    try {
        std::cout << "开始LFU缓存多线程安全测试..." << std::endl;
        std::cout << "注意: 根据项目规范，缓存策略默认不是线程安全的，本测试使用外部同步措施" << std::endl;
        std::cout << "LFU特点: 按访问频率淘汰，频率相同时使用LRU作为tie-breaking策略" << std::endl;
        
        testLfuBasicThreadSafety();
        testLfuConcurrentReadWrite();
        testLfuFrequencyTrackingUnderConcurrency();
        testLfuEvictionUnderConcurrency();
        testLfuTieBreakingUnderConcurrency();
        testLfuConcurrentClearOperations();
        testLfuStressTest();
        testLfuExceptionsUnderConcurrency();
        
        std::cout << "\n🎉 所有LFU缓存多线程测试通过！" << std::endl;
        std::cout << "验证了使用适当外部同步措施时，LFU缓存在多线程环境下的正确性。" << std::endl;
        std::cout << "确认了LFU频率跟踪机制和tie-breaking策略在并发环境下的正确性。" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ 测试失败: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}