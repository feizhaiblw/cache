#ifndef LRU_K_CACHE_H
#define LRU_K_CACHE_H

#include "CachePolicy.h"
#include <unordered_map>
#include <vector>
#include <list>
#include <stdexcept>
#include <string>
#include <chrono>
#include <atomic>
#include <shared_mutex>
#include <thread>

/**
 * @brief LRU-K缓存策略实现
 * 
 * LRU-K算法跟踪每个页面的最近K次访问时间，淘汰第K次访问时间最早的页面。
 * 当页面的访问次数少于K次时，使用FIFO策略。
 * 
 * @tparam Key 缓存键的类型
 * @tparam Value 缓存值的类型
 */
template<typename Key, typename Value>
class LruKCache : public CachePolicy<Key, Value> {
private:
    using TimeStamp = std::chrono::steady_clock::time_point;
    
    /**
     * @brief 历史记录节点（仅记录访问历史，不存储数据）
     */
    struct HistoryRecord {
        Key key;                           // 键
        std::list<TimeStamp> access_times; // 访问时间历史
        
        explicit HistoryRecord(const Key& k) : key(k) {}
        explicit HistoryRecord(Key&& k) : key(std::move(k)) {}
    };
    
    /**
     * @brief 缓存记录节点（存储实际数据）
     */
    struct CacheRecord {
        Key key;                           // 键
        Value value;                       // 值
        std::list<TimeStamp> access_times; // 访问时间历史（最多保存K个）
        
        CacheRecord(const Key& k, const Value& v) : key(k), value(v) {}
        CacheRecord(Key&& k, Value&& v) : key(std::move(k)), value(std::move(v)) {}
    };
    
    using HistoryPtr = std::shared_ptr<HistoryRecord>;
    using CachePtr = std::shared_ptr<CacheRecord>;
    
    std::atomic<int> capacity_;                             // 原子缓存容量
    std::atomic<int> k_;                                   // 原子K值
    std::unordered_map<Key, HistoryPtr> history_;          // 历史队列：记录所有访问历史
    std::unordered_map<Key, CachePtr> cache_;              // 缓存队列：只存储访问次数≥K的数据
    mutable std::shared_mutex mutex_;                       // 读写锁，保证线程安全
    
    /**
     * @brief 获取当前时间戳
     */
    TimeStamp getCurrentTime() const;
    
    /**
     * @brief 记录访问时间到历史队列
     * @param key 访问的键
     */
    void recordHistoryAccess(const Key& key);
    
    /**
     * @brief 记录访问时间到缓存队列
     * @param record 缓存记录
     */
    void recordCacheAccess(CachePtr record);
    
    /**
     * @brief 将数据从历史队列提升到缓存队列
     * @param key 键
     * @param value 值
     */
    void promoteToCache(const Key& key, const Value& value);
    
    /**
     * @brief 查找要淘汰的键
     * @return 要淘汰的键
     */
    Key findVictimKey() const;
    
    /**
     * @brief 获取历史记录的第K次访问时间
     * @param record 历史记录
     * @return 第K次访问时间
     */
    TimeStamp getKthAccessTime(const HistoryPtr& record) const;
    
    /**
     * @brief 获取缓存记录的第K次访问时间
     * @param record 缓存记录
     * @return 第K次访问时间
     */
    TimeStamp getCacheKthAccessTime(const CachePtr& record) const;
    
public:
    /**
     * @brief 构造函数
     * 
     * @param capacity 缓存容量，必须大于0
     * @param k K值，必须大于0，默认为2
     * @throws InvalidCapacityException 如果容量无效
     * @throws std::invalid_argument 如果K值无效
     */
    explicit LruKCache(int capacity, int k = 2);
    
    /**
     * @brief 析构函数
     */
    ~LruKCache() override = default;
    
    /**
     * @brief 获取指定键的值
     * 
     * @param key 要查找的键
     * @return Value 对应的值
     * @throws std::out_of_range 如果键不存在
     */
    Value get(const Key& key) override;
    
    /**
     * @brief 插入或更新键值对
     * 
     * @param key 要插入或更新的键
     * @param value 对应的值
     */
    void put(const Key& key, const Value& value) override;
    
    /**
     * @brief 检查指定键是否存在于缓存中
     * 
     * @param key 要检查的键
     * @return true 如果键存在
     * @return false 如果键不存在
     */
    bool contains(const Key& key) const override;
    
    /**
     * @brief 获取当前缓存中元素的数量
     * 
     * @return int 当前缓存大小
     */
    int size() const override {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return static_cast<int>(cache_.size());
    }
    
    /**
     * @brief 获取缓存的最大容量
     * 
     * @return int 缓存容量
     */
    int capacity() const override {
        return capacity_.load(std::memory_order_acquire);
    }
    
    /**
     * @brief 检查缓存是否为空
     * 
     * @return true 如果缓存为空
     * @return false 如果缓存不为空
     */
    bool empty() const override {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return cache_.empty();
    }
    
    /**
     * @brief 清空缓存中的所有元素
     */
    void clear() override;
    
    /**
     * @brief 获取缓存策略的名称
     * 
     * @return std::string 策略名称
     */
    std::string getPolicyName() const override;
    
    /**
     * @brief 获取K值
     * 
     * @return int K值
     */
    int getK() const { return k_.load(std::memory_order_acquire); }
    
    /**
     * @brief 获取指定键在历史队列中的访问次数
     * 
     * @param key 要查询的键
     * @return int 历史访问次数，如果键不存在返回0
     */
    int getHistoryAccessCount(const Key& key) const;
    
    /**
     * @brief 获取指定键在缓存队列中的访问次数
     * 
     * @param key 要查询的键
     * @return int 缓存访问次数，如果键不存在返回0
     */
    int getCacheAccessCount(const Key& key) const;
};

#endif // LRU_K_CACHE_H