#include "../include/LfuCache.h"
#include <stdexcept>
#include <mutex>

// FrequencyBucket的方法实现
template<typename Key, typename Value>
void LfuCache<Key, Value>::FrequencyBucket::addToHead(CacheNode* node) {
    node->prev = head;
    node->next = head->next;
    head->next->prev = node;
    head->next = node;
    size++;
}

template<typename Key, typename Value>
void LfuCache<Key, Value>::FrequencyBucket::removeNode(CacheNode* node) {
    node->prev->next = node->next;
    node->next->prev = node->prev;
    size--;
}

template<typename Key, typename Value>
typename LfuCache<Key, Value>::CacheNode* 
LfuCache<Key, Value>::FrequencyBucket::removeTail() {
    if (size == 0) {
        return nullptr;
    }
    
    CacheNode* last = tail->prev;
    
    // 确保不是头节点
    if (last == head) {
        return nullptr;
    }
    
    removeNode(last);
    return last;
}

template<typename Key, typename Value>
LfuCache<Key, Value>::LfuCache(int capacity) : capacity_(capacity), min_frequency_(1) {
    if (capacity <= 0) {
        throw InvalidCapacityException(capacity);
    }
}

template<typename Key, typename Value>
LfuCache<Key, Value>::~LfuCache() {
    clear();
}

template<typename Key, typename Value>
typename LfuCache<Key, Value>::FrequencyBucket* 
LfuCache<Key, Value>::getOrCreateBucket(int frequency) {
    auto it = frequency_buckets_.find(frequency);
    if (it == frequency_buckets_.end()) {
        frequency_buckets_[frequency] = std::make_unique<FrequencyBucket>();
    }
    return frequency_buckets_[frequency].get();
}

template<typename Key, typename Value>
void LfuCache<Key, Value>::updateFrequency(NodePtr node) {
    int old_freq = node->frequency;
    int new_freq = old_freq + 1;
    
    // 从旧频率桶中移除
    auto old_bucket_it = frequency_buckets_.find(old_freq);
    if (old_bucket_it != frequency_buckets_.end()) {
        FrequencyBucket* old_bucket = old_bucket_it->second.get();
        old_bucket->removeNode(node);
        
        // 如果旧频率桶变空且是最小频率，更新最小频率
        if (old_bucket->empty() && old_freq == min_frequency_) {
            min_frequency_++;
        }
    }
    
    // 更新节点频率
    node->frequency = new_freq;
    
    // 添加到新频率桶
    FrequencyBucket* new_bucket = getOrCreateBucket(new_freq);
    new_bucket->addToHead(node);
}

template<typename Key, typename Value>
typename LfuCache<Key, Value>::NodePtr LfuCache<Key, Value>::evictLFU() {
    // 找到最小频率的桶
    auto it = frequency_buckets_.find(min_frequency_);
    if (it == frequency_buckets_.end() || it->second->empty()) {
        return nullptr;
    }
    
    // 从最小频率桶的尾部移除节点（LRU策略）
    FrequencyBucket* bucket = it->second.get();
    NodePtr evicted = bucket->removeTail();
    
    return evicted;
}

template<typename Key, typename Value>
Value LfuCache<Key, Value>::get(const Key& key) {
    std::shared_lock<std::shared_mutex> read_lock(mutex_);
    auto it = cache_.find(key);
    
    if (it == cache_.end()) {
        throw std::out_of_range("Key not found in LFU cache");
    }
    
    NodePtr node = it->second;
    
    // 释放读锁，获取写锁来更新频率
    read_lock.unlock();
    std::unique_lock<std::shared_mutex> write_lock(mutex_);
    
    // 更新访问频率
    updateFrequency(node);
    
    return node->value;
}

template<typename Key, typename Value>
void LfuCache<Key, Value>::put(const Key& key, const Value& value) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    if (capacity_ == 0) {
        return;
    }
    
    auto it = cache_.find(key);
    
    if (it != cache_.end()) {
        // 键已存在，更新值并增加频率
        NodePtr node = it->second;
        node->value = value;
        // put操作也算作一次访问，增加频率
        updateFrequency(node);
    } else {
        // 键不存在，需要插入新节点
        
        // 如果缓存已满，先淘汰
        if (cache_.size() >= static_cast<size_t>(capacity_)) {
            NodePtr evicted = evictLFU();
            if (evicted) {
                cache_.erase(evicted->key);
                delete evicted;
            }
        }
        
        // 创建新节点
        NodePtr new_node = new CacheNode(key, value, 1);
        
        // 添加到频率为1的桶
        FrequencyBucket* bucket = getOrCreateBucket(1);
        bucket->addToHead(new_node);
        
        // 更新最小频率
        min_frequency_ = 1;
        
        // 添加到缓存映射
        cache_[key] = new_node;
    }
}

template<typename Key, typename Value>
bool LfuCache<Key, Value>::contains(const Key& key) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return cache_.find(key) != cache_.end();
}

template<typename Key, typename Value>
void LfuCache<Key, Value>::clear() {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    // 删除所有节点
    for (auto& pair : cache_) {
        delete pair.second;
    }
    cache_.clear();
    
    // 清空频率桶
    frequency_buckets_.clear();
    
    // 重置最小频率
    min_frequency_ = 1;
}

template<typename Key, typename Value>
std::string LfuCache<Key, Value>::getPolicyName() const {
    return "LFU";
}

template<typename Key, typename Value>
int LfuCache<Key, Value>::getFrequency(const Key& key) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    auto it = cache_.find(key);
    if (it == cache_.end()) {
        return 0;
    }
    
    return it->second->frequency;
}



// 显式实例化模板，支持常用类型
template class LfuCache<int, int>;
template class LfuCache<int, std::string>;
template class LfuCache<std::string, int>;
template class LfuCache<std::string, std::string>;