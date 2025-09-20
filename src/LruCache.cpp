#include "../include/LruCache.h"
#include <stdexcept>
#include <mutex>

template<typename Key, typename Value>
LruCache<Key, Value>::LruCache(int capacity) : capacity_(capacity), size_(0) {
    if (capacity <= 0) {
        throw InvalidCapacityException(capacity);
    }
    
    // 创建虚拟头节点和尾节点
    head_ = new LruNode();
    tail_ = new LruNode();
    
    // 初始化双向链表
    head_->next = tail_;
    tail_->prev = head_;
}

template<typename Key, typename Value>
LruCache<Key, Value>::~LruCache() {
    clear();
    delete head_;
    delete tail_;
}

template<typename Key, typename Value>
void LruCache<Key, Value>::addToHead(LruNodePtr node) {
    // 在头部添加节点
    node->prev = head_;
    node->next = head_->next;
    head_->next->prev = node;
    head_->next = node;
}

template<typename Key, typename Value>
void LruCache<Key, Value>::removeNode(LruNodePtr node) {
    // 删除指定节点
    node->prev->next = node->next;
    node->next->prev = node->prev;
}

template<typename Key, typename Value>
void LruCache<Key, Value>::moveToHead(LruNodePtr node) {
    // 先移除，再添加到头部
    removeNode(node);
    addToHead(node);
}

template<typename Key, typename Value>
typename LruCache<Key, Value>::LruNodePtr LruCache<Key, Value>::removeTail() {
    // 获取尾部的最后一个真实节点
    LruNodePtr last = tail_->prev;
    removeNode(last);
    return last;
}

template<typename Key, typename Value>
Value LruCache<Key, Value>::get(const Key& key) {
    std::shared_lock<std::shared_mutex> read_lock(mutex_);
    auto it = cache_.find(key);
    
    if (it == cache_.end()) {
        throw std::out_of_range("Key not found in LRU cache");
    }
    
    auto node = it->second;
    
    // 释放读锁，获取写锁来更新位置
    read_lock.unlock();
    std::unique_lock<std::shared_mutex> write_lock(mutex_);
    
    // 移动到头部（标记为最近使用）
    moveToHead(node);
    
    return node->value;
}

template<typename Key, typename Value>
void LruCache<Key, Value>::put(const Key& key, const Value& value) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    auto it = cache_.find(key);
    
    if (it != cache_.end()) {
        // 键已存在，更新值并移动到头部
        auto node = it->second;
        node->value = value;
        moveToHead(node);
    } else {
        // 键不存在，需要插入新节点
        auto new_node = new LruNode(key, value);
        
        if (size_ >= capacity_) {
            // 缓存已满，移除最久未使用的节点
            auto tail_node = removeTail();
            cache_.erase(tail_node->key);
            size_--;
            delete tail_node;
        }
        
        // 添加新节点到头部
        addToHead(new_node);
        cache_[key] = new_node;
        size_++;
    }
}

template<typename Key, typename Value>
bool LruCache<Key, Value>::contains(const Key& key) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return cache_.find(key) != cache_.end();
}

template<typename Key, typename Value>
void LruCache<Key, Value>::clear() {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    // 删除所有节点
    for (auto& pair : cache_) {
        delete pair.second;
    }
    
    cache_.clear();
    size_ = 0;
    
    // 重置头尾指针
    head_->next = tail_;
    tail_->prev = head_;
}

// 显式实例化模板，支持常用类型
template class LruCache<int, int>;
template class LruCache<int, std::string>;
template class LruCache<std::string, int>;
template class LruCache<std::string, std::string>;