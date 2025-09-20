#ifndef LRU_CACHE_H
#define LRU_CACHE_H

#include "CachePolicy.h"
#include <unordered_map>
#include <stdexcept>
#include <string>
#include <shared_mutex>
#include <memory>

template<typename Key, typename Value>
class LruCache : public CachePolicy<Key, Value> {
private:
    // LRU节点定义 - 读写锁版本（简化）
    struct LruNode {
        Key key;                                    // 节点的键
        Value value;                               // 节点的值
        LruNode* prev;                             // 指针到前一个节点
        LruNode* next;                             // 指针到后一个节点
        
        // 默认构造函数
        LruNode() : prev(nullptr), next(nullptr) {}
        
        // 带参数的构造函数
        LruNode(const Key& k, const Value& v) 
            : key(k), value(v), prev(nullptr), next(nullptr) {}
        
        // 移动构造函数
        LruNode(Key&& k, Value&& v) 
            : key(std::move(k)), value(std::move(v)), prev(nullptr), next(nullptr) {}
        
        // 析构函数
        ~LruNode() = default;
        
        // 禁用拷贝构造和赋值操作符
        LruNode(const LruNode&) = delete;
        LruNode& operator=(const LruNode&) = delete;
    };
    
    using LruNodePtr = LruNode*;
    
    int capacity_;                                             // 缓存容量
    int size_;                                                 // 当前大小
    std::unordered_map<Key, LruNodePtr> cache_;               // 哈希表
    LruNode* head_;                                           // 头节点指针
    LruNode* tail_;                                           // 尾节点指针
    mutable std::shared_mutex mutex_;                         // 读写锁，保证线程安全
    
    // 私有辅助函数 - 读写锁版本
    void addToHead(LruNodePtr node);                         // 在头部添加节点
    void removeNode(LruNodePtr node);                        // 删除指定节点
    void moveToHead(LruNodePtr node);                        // 将节点移到头部
    LruNodePtr removeTail();                                 // 删除尾部节点
    
public:
    // 构造函数
    explicit LruCache(int capacity);
    
    // 析构函数
    ~LruCache();
    
    // 禁用拷贝构造和赋值
    LruCache(const LruCache&) = delete;
    LruCache& operator=(const LruCache&) = delete;
    
    // 获取值
    Value get(const Key& key) override;
    
    // 插入或更新键值对
    void put(const Key& key, const Value& value) override;
    
    // 检查键是否存在
    bool contains(const Key& key) const override;
    
    // 获取当前大小
    int size() const override { 
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return size_; 
    }
    
    // 获取容量
    int capacity() const override { 
        return capacity_; 
    }
    
    // 检查是否为空
    bool empty() const override { 
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return size_ == 0; 
    }
    
    // 清空缓存
    void clear() override;
    
    // 获取策略名称
    std::string getPolicyName() const override { return "LRU"; }
};

#endif // LRU_CACHE_H