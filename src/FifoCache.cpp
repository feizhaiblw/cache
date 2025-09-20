#include "../include/FifoCache.h"
#include <stdexcept>
#include <mutex>

template<typename Key, typename Value>
FifoCache<Key, Value>::FifoCache(int capacity) : capacity_(capacity), size_(0) {
    if (capacity <= 0) {
        throw InvalidCapacityException(capacity);
    }
    
    // 初始化头尾指针为空
    head_ = nullptr;
    tail_ = nullptr;
}

template<typename Key, typename Value>
FifoCache<Key, Value>::~FifoCache() {
    clear();
}

template<typename Key, typename Value>
void FifoCache<Key, Value>::addToTail(FifoNodePtr node) {
    if (tail_ == nullptr) {
        // 第一个节点
        head_ = node;
        tail_ = node;
    } else {
        // 添加到尾部
        tail_->next = node;
        tail_ = node;
    }
}

template<typename Key, typename Value>
typename FifoCache<Key, Value>::FifoNodePtr FifoCache<Key, Value>::removeHead() {
    if (head_ == nullptr) {
        return nullptr;
    }
    
    FifoNodePtr old_head = head_;
    head_ = head_->next;
    
    if (head_ == nullptr) {
        // 最后一个节点
        tail_ = nullptr;
    }
    
    return old_head;
}

template<typename Key, typename Value>
Value FifoCache<Key, Value>::get(const Key& key) {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    auto it = cache_.find(key);
    
    if (it == cache_.end()) {
        throw std::out_of_range("Key not found in FIFO cache");
    }
    
    return it->second->value;
}

template<typename Key, typename Value>
void FifoCache<Key, Value>::put(const Key& key, const Value& value) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    auto it = cache_.find(key);
    
    if (it != cache_.end()) {
        // 键已存在，直接更新值
        it->second->value = value;
    } else {
        // 键不存在，需要插入新节点
        auto new_node = new FifoNode(key, value);
        
        if (size_ >= capacity_) {
            // 缓存已满，移除最早的节点
            auto head_node = removeHead();
            if (head_node) {
                cache_.erase(head_node->key);
                size_--;
                delete head_node;
            }
        }
        
        // 添加新节点到尾部
        addToTail(new_node);
        cache_[key] = new_node;
        size_++;
    }
}

template<typename Key, typename Value>
bool FifoCache<Key, Value>::contains(const Key& key) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return cache_.find(key) != cache_.end();
}

template<typename Key, typename Value>
void FifoCache<Key, Value>::clear() {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    // 删除所有节点
    for (auto& pair : cache_) {
        delete pair.second;
    }
    
    cache_.clear();
    size_ = 0;
    
    // 重置头尾指针
    head_ = nullptr;
    tail_ = nullptr;
}

template<typename Key, typename Value>
std::string FifoCache<Key, Value>::getPolicyName() const {
    return "FIFO";
}

// 显式实例化模板，支持常用类型
template class FifoCache<int, int>;
template class FifoCache<int, std::string>;
template class FifoCache<std::string, int>;
template class FifoCache<std::string, std::string>;