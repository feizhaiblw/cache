#ifndef FIFO_CACHE_H
#define FIFO_CACHE_H

#include "CachePolicy.h"
#include <unordered_map>
#include <stdexcept>
#include <string>
#include <shared_mutex>
#include <memory>

/**
 * @brief FIFO（先进先出）缓存策略实现
 * 
 * 当缓存满时，移除最早插入的元素
 * 
 * @tparam Key 缓存键的类型
 * @tparam Value 缓存值的类型
 */
template<typename Key, typename Value>
class FifoCache : public CachePolicy<Key, Value> {
private:
    // FIFO节点定义 - 读写锁版本
    struct FifoNode {
        Key key;
        Value value;
        FifoNode* next;                       // 指针到下一个节点
        
        // 默认构造函数
        FifoNode() : next(nullptr) {}
        
        // 带参数的构造函数
        FifoNode(const Key& k, const Value& v) 
            : key(k), value(v), next(nullptr) {}
        
        // 移动构造函数
        FifoNode(Key&& k, Value&& v) 
            : key(std::move(k)), value(std::move(v)), next(nullptr) {}
        
        // 禁用拷贝构造和赋值操作符
        FifoNode(const FifoNode&) = delete;
        FifoNode& operator=(const FifoNode&) = delete;
    };
    
    using FifoNodePtr = FifoNode*;
    
    int capacity_;                                           // 缓存容量
    int size_;                                               // 当前大小
    std::unordered_map<Key, FifoNodePtr> cache_;            // 哈希表
    FifoNode* head_;                                         // 头节点指针（最旧的）
    FifoNode* tail_;                                         // 尾节点指针（最新的）
    mutable std::shared_mutex mutex_;                        // 读写锁，保证线程安全
    
    // 私有辅助函数 - 读写锁版本
    void addToTail(FifoNodePtr node);                       // 在尾部添加节点
    FifoNodePtr removeHead();                               // 删除头部节点
    
public:
    /**
     * @brief 构造函数
     * 
     * @param capacity 缓存容量，必须大于0
     * @throws InvalidCapacityException 如果容量无效
     */
    explicit FifoCache(int capacity);
    
    /**
     * @brief 析构函数
     */
    ~FifoCache() override;
    
    /**
     * @brief 禁用拷贝构造和赋值
     */
    FifoCache(const FifoCache&) = delete;
    FifoCache& operator=(const FifoCache&) = delete;
    
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
        return size_;
    }
    
    /**
     * @brief 获取缓存的最大容量
     * 
     * @return int 缓存容量
     */
    int capacity() const override {
        return capacity_;
    }
    
    /**
     * @brief 检查缓存是否为空
     * 
     * @return true 如果缓存为空
     * @return false 如果缓存不为空
     */
    bool empty() const override {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return size_ == 0;
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
};

#endif // FIFO_CACHE_H