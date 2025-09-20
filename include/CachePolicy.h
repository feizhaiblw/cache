#ifndef CACHE_POLICY_H
#define CACHE_POLICY_H

#include <memory>
#include <stdexcept>
#include <atomic>
#include <thread>

/**
 * @brief 缓存策略的抽象基类
 * 
 * 这个抽象类定义了所有缓存淘汰策略必须实现的接口。
 * 当前支持的策略包括：
 * - LRU (Least Recently Used): 最近最少使用策略
 * - LFU (Least Frequently Used): 最少使用频率策略
 * - FIFO (First In First Out): 先进先出策略
 * - LRU-K: 基于K次访问历史的LRU策略
 * 
 * 所有实现均支持多线程安全访问，采用原子操作和无锁数据结构。
 * 
 * @tparam Key 缓存键的类型
 * @tparam Value 缓存值的类型
 */
template<typename Key, typename Value>
class CachePolicy {
public:
    /**
     * @brief 虚析构函数，确保派生类正确析构
     */
    virtual ~CachePolicy() = default;

    /**
     * @brief 获取指定键的值
     * 
     * @param key 要查找的键
     * @return Value 对应的值
     * @throws std::out_of_range 如果键不存在
     */
    virtual Value get(const Key& key) = 0;

    /**
     * @brief 插入或更新键值对
     * 
     * 如果缓存已满，将根据具体的淘汰策略移除某个元素
     * 
     * @param key 要插入或更新的键
     * @param value 对应的值
     */
    virtual void put(const Key& key, const Value& value) = 0;

    /**
     * @brief 检查指定键是否存在于缓存中
     * 
     * @param key 要检查的键
     * @return true 如果键存在
     * @return false 如果键不存在
     */
    virtual bool contains(const Key& key) const = 0;

    /**
     * @brief 获取当前缓存中元素的数量
     * 
     * @return int 当前缓存大小
     */
    virtual int size() const = 0;

    /**
     * @brief 获取缓存的最大容量
     * 
     * @return int 缓存容量
     */
    virtual int capacity() const = 0;

    /**
     * @brief 检查缓存是否为空
     * 
     * @return true 如果缓存为空
     * @return false 如果缓存不为空
     */
    virtual bool empty() const = 0;

    /**
     * @brief 清空缓存中的所有元素
     */
    virtual void clear() = 0;

    /**
     * @brief 获取缓存策略的名称
     * 
     * @return std::string 策略名称（如"LRU"、"LFU"、"FIFO"、"LRU-K"等）
     */
    virtual std::string getPolicyName() const = 0;

protected:
    /**
     * @brief 受保护的默认构造函数
     * 防止直接实例化抽象类
     */
    CachePolicy() = default;

    /**
     * @brief 禁用拷贝构造函数
     */
    CachePolicy(const CachePolicy&) = delete;

    /**
     * @brief 禁用拷贝赋值操作符
     */
    CachePolicy& operator=(const CachePolicy&) = delete;
};

/**
 * @brief 缓存策略工厂类
 * 
 * 用于创建不同类型的缓存策略实例。
 * 支持创建当前项目中实现的所有缓存策略。
 */
template<typename Key, typename Value>
class CachePolicyFactory {
public:
    /**
     * @brief 缓存策略类型枚举
     * 
     * 定义了当前项目支持的所有缓存淘汰策略类型
     */
    enum class PolicyType {
        LRU,    // 最近最少使用 (Least Recently Used)
        LFU,    // 最少使用频率 (Least Frequently Used)
        FIFO,   // 先进先出 (First In First Out)
        LRU_K   // LRU-K算法，基于K次访问历史的LRU策略
    };

    /**
     * @brief 创建指定类型的缓存策略
     * 
     * 支持创建以下策略类型：
     * - PolicyType::LRU: 创建 LruCache 实例
     * - PolicyType::LFU: 创建 LfuCache 实例
     * - PolicyType::FIFO: 创建 FifoCache 实例
     * - PolicyType::LRU_K: 创建 LruKCache 实例（默认K=2）
     * 
     * @param type 策略类型
     * @param capacity 缓存容量，必须大于0
     * @return std::unique_ptr<CachePolicy<Key, Value>> 策略实例
     * @throws InvalidCapacityException 如果容量无效
     */
    static std::unique_ptr<CachePolicy<Key, Value>> createPolicy(
        PolicyType type, int capacity);

private:
    CachePolicyFactory() = delete;  // 工厂类不允许实例化
};

/**
 * @brief 缓存策略异常类
 */
class CachePolicyException : public std::runtime_error {
public:
    explicit CachePolicyException(const std::string& message) 
        : std::runtime_error("CachePolicy Error: " + message) {}
};

/**
 * @brief 缓存容量异常类
 */
class InvalidCapacityException : public CachePolicyException {
public:
    explicit InvalidCapacityException(int capacity) 
        : CachePolicyException("Invalid capacity: " + std::to_string(capacity) + 
                              ". Capacity must be greater than 0.") {}
};

#endif // CACHE_POLICY_H