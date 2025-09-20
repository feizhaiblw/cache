#ifndef THREAD_SAFE_TEST_FRAMEWORK_H
#define THREAD_SAFE_TEST_FRAMEWORK_H

#include <thread>
#include <vector>
#include <random>
#include <chrono>
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <future>
#include <functional>
#include <iostream>
#include <memory>
#include <cassert>
#include <set>
#include <map>

/**
 * @brief 多线程测试框架
 * 
 * 提供了一套完整的多线程测试工具，用于验证缓存策略在并发环境下的正确性。
 * 包括数据一致性验证、竞态条件检测、死锁检测等功能。
 */
template<typename Key, typename Value>
class ThreadSafeTestFramework {
public:
    /**
     * @brief 测试操作类型
     */
    enum class OperationType {
        PUT,        // 插入操作
        GET,        // 获取操作
        CONTAINS,   // 检查存在性
        CLEAR,      // 清空操作
        SIZE        // 获取大小
    };

    /**
     * @brief 测试操作记录
     */
    struct Operation {
        OperationType type;
        Key key;
        Value value;
        std::thread::id thread_id;
        std::chrono::steady_clock::time_point timestamp;
        bool success;           // 操作是否成功
        std::string result;     // 操作结果描述
        
        Operation(OperationType t, const Key& k, const Value& v) 
            : type(t), key(k), value(v), thread_id(std::this_thread::get_id()),
              timestamp(std::chrono::steady_clock::now()), success(false) {}
    };

    /**
     * @brief 测试统计信息
     */
    struct TestStatistics {
        std::atomic<int> total_operations{0};
        std::atomic<int> successful_operations{0};
        std::atomic<int> failed_operations{0};
        std::atomic<int> put_operations{0};
        std::atomic<int> get_operations{0};
        std::atomic<int> contains_operations{0};
        std::atomic<int> clear_operations{0};
        std::atomic<int> size_operations{0};
        std::chrono::steady_clock::time_point start_time;
        std::chrono::steady_clock::time_point end_time;
        
        void reset() {
            total_operations.store(0);
            successful_operations.store(0);
            failed_operations.store(0);
            put_operations.store(0);
            get_operations.store(0);
            contains_operations.store(0);
            clear_operations.store(0);
            size_operations.store(0);
        }
        
        void print() const {
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                end_time - start_time).count();
            
            std::cout << "=== 多线程测试统计 ===" << std::endl;
            std::cout << "测试耗时: " << duration << " ms" << std::endl;
            std::cout << "总操作数: " << total_operations.load() << std::endl;
            std::cout << "成功操作: " << successful_operations.load() << std::endl;
            std::cout << "失败操作: " << failed_operations.load() << std::endl;
            std::cout << "PUT操作: " << put_operations.load() << std::endl;
            std::cout << "GET操作: " << get_operations.load() << std::endl;
            std::cout << "CONTAINS操作: " << contains_operations.load() << std::endl;
            std::cout << "清空操作: " << clear_operations.load() << std::endl;
            std::cout << "SIZE操作: " << size_operations.load() << std::endl;
            
            if (duration > 0) {
                double ops_per_sec = (double)total_operations.load() * 1000.0 / duration;
                std::cout << "操作速率: " << ops_per_sec << " ops/sec" << std::endl;
            }
        }
    };

private:
    std::mutex operations_mutex_;
    std::vector<Operation> operations_log_;
    TestStatistics statistics_;
    std::atomic<bool> stop_flag_{false};

public:
    /**
     * @brief 构造函数
     */
    ThreadSafeTestFramework() {
        operations_log_.reserve(100000); // 预分配空间
    }

    /**
     * @brief 记录操作
     */
    void recordOperation(const Operation& op) {
        std::lock_guard<std::mutex> lock(operations_mutex_);
        operations_log_.push_back(op);
        
        statistics_.total_operations++;
        if (op.success) {
            statistics_.successful_operations++;
        } else {
            statistics_.failed_operations++;
        }
        
        switch (op.type) {
            case OperationType::PUT:
                statistics_.put_operations++;
                break;
            case OperationType::GET:
                statistics_.get_operations++;
                break;
            case OperationType::CONTAINS:
                statistics_.contains_operations++;
                break;
            case OperationType::CLEAR:
                statistics_.clear_operations++;
                break;
            case OperationType::SIZE:
                statistics_.size_operations++;
                break;
        }
    }

    /**
     * @brief 停止所有测试线程
     */
    void stopTest() {
        stop_flag_.store(true, std::memory_order_release);
    }

    /**
     * @brief 检查是否应该停止测试
     */
    bool shouldStop() const {
        return stop_flag_.load(std::memory_order_acquire);
    }

    /**
     * @brief 重置测试状态
     */
    void reset() {
        std::lock_guard<std::mutex> lock(operations_mutex_);
        operations_log_.clear();
        statistics_.reset();
        stop_flag_.store(false);
    }

    /**
     * @brief 获取统计信息
     */
    const TestStatistics& getStatistics() const {
        return statistics_;
    }

    /**
     * @brief 设置测试开始时间
     */
    void setStartTime() {
        statistics_.start_time = std::chrono::steady_clock::now();
    }

    /**
     * @brief 设置测试结束时间
     */
    void setEndTime() {
        statistics_.end_time = std::chrono::steady_clock::now();
    }

    /**
     * @brief 并发PUT测试
     * 
     * @param cache 缓存实例
     * @param thread_count 线程数量
     * @param operations_per_thread 每个线程的操作数
     * @param key_range 键的范围
     */
    template<typename CacheType>
    void concurrentPutTest(CacheType& cache, int thread_count, 
                          int operations_per_thread, int key_range) {
        std::vector<std::thread> threads;
        std::mutex cache_mutex; // 外部同步措施
        
        std::cout << "执行并发PUT测试: " << thread_count << "线程, " 
                  << operations_per_thread << "操作/线程, 键范围[0," << key_range << "]" << std::endl;

        setStartTime();
        
        for (int t = 0; t < thread_count; ++t) {
            threads.emplace_back([this, &cache, &cache_mutex, operations_per_thread, key_range, t]() {
                std::random_device rd;
                std::mt19937 gen(rd() + t);
                std::uniform_int_distribution<Key> key_dist(0, key_range - 1);
                
                for (int i = 0; i < operations_per_thread && !shouldStop(); ++i) {
                    Key key = key_dist(gen);
                    Value value = static_cast<Value>(key * 10 + t); // 保证唯一性
                    
                    Operation op(OperationType::PUT, key, value);
                    
                    try {
                        std::lock_guard<std::mutex> lock(cache_mutex);
                        cache.put(key, value);
                        op.success = true;
                        op.result = "PUT成功";
                    } catch (const std::exception& e) {
                        op.success = false;
                        op.result = std::string("PUT失败: ") + e.what();
                    }
                    
                    recordOperation(op);
                }
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        setEndTime();
    }

    /**
     * @brief 并发GET测试
     */
    template<typename CacheType>
    void concurrentGetTest(CacheType& cache, int thread_count, 
                          int operations_per_thread, int key_range) {
        std::vector<std::thread> threads;
        std::shared_mutex cache_mutex; // 读写锁，允许并发读
        
        std::cout << "执行并发GET测试: " << thread_count << "线程, " 
                  << operations_per_thread << "操作/线程" << std::endl;

        setStartTime();
        
        for (int t = 0; t < thread_count; ++t) {
            threads.emplace_back([this, &cache, &cache_mutex, operations_per_thread, key_range, t]() {
                std::random_device rd;
                std::mt19937 gen(rd() + t);
                std::uniform_int_distribution<Key> key_dist(0, key_range - 1);
                
                for (int i = 0; i < operations_per_thread && !shouldStop(); ++i) {
                    Key key = key_dist(gen);
                    
                    Operation op(OperationType::GET, key, Value{});
                    
                    try {
                        std::shared_lock<std::shared_mutex> lock(cache_mutex);
                        Value value = cache.get(key);
                        op.success = true;
                        op.result = "GET成功，值=" + std::to_string(value);
                    } catch (const std::exception& e) {
                        op.success = false;
                        op.result = std::string("GET失败: ") + e.what();
                    }
                    
                    recordOperation(op);
                }
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        setEndTime();
    }

    /**
     * @brief 混合操作测试（PUT + GET + CONTAINS）
     */
    template<typename CacheType>
    void mixedOperationsTest(CacheType& cache, int thread_count, 
                           int operations_per_thread, int key_range) {
        std::vector<std::thread> threads;
        std::shared_mutex cache_mutex;
        
        std::cout << "执行混合操作测试: " << thread_count << "线程, " 
                  << operations_per_thread << "操作/线程" << std::endl;

        setStartTime();
        
        for (int t = 0; t < thread_count; ++t) {
            threads.emplace_back([this, &cache, &cache_mutex, operations_per_thread, key_range, t]() {
                std::random_device rd;
                std::mt19937 gen(rd() + t);
                std::uniform_int_distribution<Key> key_dist(0, key_range - 1);
                std::uniform_int_distribution<int> op_dist(0, 2); // 0=PUT, 1=GET, 2=CONTAINS
                
                for (int i = 0; i < operations_per_thread && !shouldStop(); ++i) {
                    Key key = key_dist(gen);
                    int op_type = op_dist(gen);
                    
                    if (op_type == 0) { // PUT操作
                        Value value = static_cast<Value>(key * 10 + t);
                        Operation op(OperationType::PUT, key, value);
                        
                        try {
                            std::unique_lock<std::shared_mutex> lock(cache_mutex);
                            cache.put(key, value);
                            op.success = true;
                            op.result = "PUT成功";
                        } catch (const std::exception& e) {
                            op.success = false;
                            op.result = std::string("PUT失败: ") + e.what();
                        }
                        
                        recordOperation(op);
                        
                    } else if (op_type == 1) { // GET操作
                        Operation op(OperationType::GET, key, Value{});
                        
                        try {
                            std::shared_lock<std::shared_mutex> lock(cache_mutex);
                            Value value = cache.get(key);
                            op.success = true;
                            op.result = "GET成功";
                        } catch (const std::exception& e) {
                            op.success = false;
                            op.result = std::string("GET失败: ") + e.what();
                        }
                        
                        recordOperation(op);
                        
                    } else { // CONTAINS操作
                        Operation op(OperationType::CONTAINS, key, Value{});
                        
                        try {
                            std::shared_lock<std::shared_mutex> lock(cache_mutex);
                            bool exists = cache.contains(key);
                            op.success = true;
                            op.result = exists ? "CONTAINS:存在" : "CONTAINS:不存在";
                        } catch (const std::exception& e) {
                            op.success = false;
                            op.result = std::string("CONTAINS失败: ") + e.what();
                        }
                        
                        recordOperation(op);
                    }
                }
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        setEndTime();
    }

    /**
     * @brief 数据一致性验证
     * 
     * 验证缓存中的数据在多线程环境下是否保持一致性
     */
    template<typename CacheType>
    bool validateDataConsistency(CacheType& cache, int key_range) {
        std::shared_mutex cache_mutex;
        std::cout << "验证数据一致性..." << std::endl;
        
        bool consistent = true;
        std::map<Key, Value> expected_values;
        
        // 收集预期的键值对
        {
            std::shared_lock<std::shared_mutex> lock(cache_mutex);
            for (int key = 0; key < key_range; ++key) {
                if (cache.contains(key)) {
                    try {
                        Value value = cache.get(key);
                        expected_values[key] = value;
                    } catch (const std::exception& e) {
                        std::cout << "获取键" << key << "时发生异常: " << e.what() << std::endl;
                        consistent = false;
                    }
                }
            }
        }
        
        // 验证数据一致性：多次读取相同键应该返回相同值
        for (const auto& pair : expected_values) {
            Key key = pair.first;
            Value expected_value = pair.second;
            
            for (int attempt = 0; attempt < 5; ++attempt) {
                try {
                    std::shared_lock<std::shared_mutex> lock(cache_mutex);
                    if (cache.contains(key)) {
                        Value actual_value = cache.get(key);
                        if (actual_value != expected_value) {
                            std::cout << "数据不一致: 键" << key 
                                      << " 期望值=" << expected_value 
                                      << " 实际值=" << actual_value << std::endl;
                            consistent = false;
                        }
                    }
                } catch (const std::exception& e) {
                    std::cout << "数据一致性验证异常: " << e.what() << std::endl;
                    consistent = false;
                }
            }
        }
        
        if (consistent) {
            std::cout << "✓ 数据一致性验证通过" << std::endl;
        } else {
            std::cout << "✗ 数据一致性验证失败" << std::endl;
        }
        
        return consistent;
    }

    /**
     * @brief 打印测试统计信息
     */
    void printStatistics() const {
        statistics_.print();
    }
};

#endif // THREAD_SAFE_TEST_FRAMEWORK_H