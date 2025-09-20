#ifndef LFU_CACHE_H
#define LFU_CACHE_H

#include "CachePolicy.h"
#include <unordered_map>
#include <unordered_set>
#include <stdexcept>
#include <string>
#include <shared_mutex>
#include <memory>

/**
 * @brief LFU（最不经常使用）缓存策略实现
 * 
 * LFU算法淘汰访问频率最低的元素。当多个元素有相同的最低频率时，
 * 淘汰其中最久未使用的元素（LRU策略作为tie-breaker）。
 * 
 * @tparam Key 缓存键的类型
 * @tparam Value 缓存值的类型
 */
template<typename Key, typename Value>
class LfuCache : public CachePolicy<Key, Value> {
private:
    /**
     * @brief 缓存节点定义 - 读写锁版本
     */
    struct CacheNode {
        Key key;                               // 键
        Value value;                           // 值
        int frequency;                         // 访问频率
        CacheNode* prev;                       // 指针到前一个节点
        CacheNode* next;                       // 指针到后一个节点
        
        CacheNode(const Key& k, const Value& v, int freq = 1) 
            : key(k), value(v), frequency(freq), prev(nullptr), next(nullptr) {}
              
        // 禁用拷贝构造和赋值
        CacheNode(const CacheNode&) = delete;
        CacheNode& operator=(const CacheNode&) = delete;
    };
    
    /**
     * @brief 频率桶定义 - 读写锁版本
     */
    struct FrequencyBucket {
        CacheNode* head;                       // 头节点指针
        CacheNode* tail;                       // 尾节点指针
        int size;                              // 桶中节点数量
        
        FrequencyBucket() : size(0) {
            auto dummy_head = new CacheNode(Key{}, Value{}, 0);
            auto dummy_tail = new CacheNode(Key{}, Value{}, 0);
            head = dummy_head;
            tail = dummy_tail;
            dummy_head->next = dummy_tail;
            dummy_tail->prev = dummy_head;
        }
        
        ~FrequencyBucket() {
            delete head;
            delete tail;
        }
        
        // 操作方法
        void addToHead(CacheNode* node);
        void removeNode(CacheNode* node);
        CacheNode* removeTail();
        bool empty() const { return size == 0; }
        
        // 禁用拷贝构造和赋值
        FrequencyBucket(const FrequencyBucket&) = delete;
        FrequencyBucket& operator=(const FrequencyBucket&) = delete;
    };
    
    using NodePtr = CacheNode*;
    using BucketPtr = std::unique_ptr<FrequencyBucket>;
    
    int capacity_;                                           // 缓存容量
    int min_frequency_;                                      // 最小频率
    std::unordered_map<Key, NodePtr> cache_;                // 键到节点的映射
    std::unordered_map<int, BucketPtr> frequency_buckets_;   // 频率到桶的映射
    mutable std::shared_mutex mutex_;                        // 读写锁，保证线程安全
    
    /**
     * @brief 更新节点的访问频率 - 读写锁版本
     * @param node 要更新的节点
     */
    void updateFrequency(NodePtr node);
    
    /**
     * @brief 淘汰最不经常使用的节点 - 读写锁版本
     * @return 被淘汰的节点
     */
    NodePtr evictLFU();
    
    /**
     * @brief 获取或创建指定频率的桶 - 读写锁版本
     * @param frequency 频率值
     * @return 频率桶指针
     */
    FrequencyBucket* getOrCreateBucket(int frequency);
    
public:
    /**
     * @brief 构造函数
     * 
     * @param capacity 缓存容量，必须大于0
     * @throws InvalidCapacityException 如果容量无效
     */
    explicit LfuCache(int capacity);
    
    /**
     * @brief 析构函数
     */
    ~LfuCache() override;
    
    /**
     * @brief 禁用拷贝构造和赋值
     */
    LfuCache(const LfuCache&) = delete;
    LfuCache& operator=(const LfuCache&) = delete;
    
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
     * @brief 获取指定键的访问频率
     * 
     * @param key 要查询的键
     * @return int 访问频率，如果键不存在返回0
     */
    int getFrequency(const Key& key) const;
    
    /**
     * @brief 获取当前最小频率
     * 
     * @return int 最小频率
     */
    int getMinFrequency() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return min_frequency_;
    }
};

#endif // LFU_CACHE_H