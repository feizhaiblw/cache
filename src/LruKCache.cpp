#include "../include/LruKCache.h"
#include "../include/LruKCache.h"
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <mutex>

template<typename Key, typename Value>
LruKCache<Key, Value>::LruKCache(int capacity, int k) : capacity_(capacity), k_(k) {
    if (capacity <= 0) {
        throw InvalidCapacityException(capacity);
    }
    if (k <= 0) {
        throw std::invalid_argument("K value must be greater than 0");
    }
}

template<typename Key, typename Value>
typename LruKCache<Key, Value>::TimeStamp LruKCache<Key, Value>::getCurrentTime() const {
    return std::chrono::steady_clock::now();
}

template<typename Key, typename Value>
void LruKCache<Key, Value>::recordHistoryAccess(const Key& key) {
    TimeStamp now = getCurrentTime();
    
    auto it = history_.find(key);
    if (it == history_.end()) {
        // 首次访问，创建历史记录
        HistoryPtr history_record = std::make_shared<HistoryRecord>(key);
        history_record->access_times.push_back(now);
        history_[key] = history_record;
    } else {
        // 追加访问时间
        it->second->access_times.push_back(now);
        
        // 如果访问次数超过K，移除最老的访问记录
        if (it->second->access_times.size() > static_cast<size_t>(k_)) {
            it->second->access_times.pop_front();
        }
    }
}

template<typename Key, typename Value>
void LruKCache<Key, Value>::recordCacheAccess(CachePtr record) {
    TimeStamp now = getCurrentTime();
    
    // 添加新的访问时间
    record->access_times.push_back(now);
    
    // 如果访问次数超过K，移除最老的访问记录
    if (record->access_times.size() > static_cast<size_t>(k_)) {
        record->access_times.pop_front();
    }
}

template<typename Key, typename Value>
void LruKCache<Key, Value>::promoteToCache(const Key& key, const Value& value) {
    // 从历史队列获取访问历史
    auto history_it = history_.find(key);
    if (history_it == history_.end()) {
        throw std::runtime_error("Key not found in history when promoting to cache");
    }
    
    // 创建缓存记录
    CachePtr cache_record = std::make_shared<CacheRecord>(key, value);
    
    // 复制访问历史
    cache_record->access_times = history_it->second->access_times;
    
    // 添加到缓存队列
    cache_[key] = cache_record;
    
     history_.erase(history_it);
}

template<typename Key, typename Value>
typename LruKCache<Key, Value>::TimeStamp 
LruKCache<Key, Value>::getKthAccessTime(const HistoryPtr& record) const {
    if (record->access_times.empty()) {
        return TimeStamp::min();
    }
    
    // 如果访问次数少于K次，返回最早的访问时间
    // 如果访问次数大于等于K次，返回第K次访问时间
    if (record->access_times.size() < static_cast<size_t>(k_)) {
        return record->access_times.front();  // 最早的访问时间
    } else {
        // list大小等于K时，back()就是第K次访问时间
        return record->access_times.back();
    }
}

template<typename Key, typename Value>
typename LruKCache<Key, Value>::TimeStamp 
LruKCache<Key, Value>::getCacheKthAccessTime(const CachePtr& record) const {
    if (record->access_times.empty()) {
        return TimeStamp::min();
    }
    
    // 缓存中的数据必然是访问次数≥K的，所以直接返回back
    return record->access_times.back();
}

template<typename Key, typename Value>
Key LruKCache<Key, Value>::findVictimKey() const {
    int k_val = k_.load(std::memory_order_acquire);
    
    // 首先尝试从历史队列中找到访问次数<K的数据进行淘汰
    Key victim_key;
    TimeStamp earliest_time = TimeStamp::max();
    bool found_in_history = false;
    
    // 优先淘汰历史队列中访问次数不足K次的数据
    for (const auto& pair : history_) {
        const Key& key = pair.first;
        const HistoryPtr& record = pair.second;
        
        if (record->access_times.size() < static_cast<size_t>(k_val)) {
            TimeStamp first_access = record->access_times.front();
            if (!found_in_history || first_access < earliest_time) {
                earliest_time = first_access;
                victim_key = key;
                found_in_history = true;
            }
        }
    }
    
    if (found_in_history) {
        return victim_key;
    }
    
    // 如果历史队列中没有访问次数<K的数据，则从缓存队列中淘汰
    if (cache_.empty()) {
        throw std::runtime_error("Cannot find victim: both history and cache are empty or no suitable candidate found");
    }
    
    earliest_time = TimeStamp::max();
    auto first_it = cache_.begin();
    victim_key = first_it->first;
    
    for (const auto& pair : cache_) {
        const Key& key = pair.first;
        const CachePtr& record = pair.second;
        
        TimeStamp kth_access = getCacheKthAccessTime(record);
        if (kth_access < earliest_time) {
            earliest_time = kth_access;
            victim_key = key;
        }
    }
    
    return victim_key;
}

template<typename Key, typename Value>
Value LruKCache<Key, Value>::get(const Key& key) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    // 首先在缓存队列中查找
    auto cache_it = cache_.find(key);
    if (cache_it != cache_.end()) {
        // 在缓存中找到，更新访问时间并返回
        recordCacheAccess(cache_it->second);
        return cache_it->second->value;
    }
    
    // 在缓存中未找到，说明数据不存在或者访问次数不足K次
    throw std::out_of_range("Key not found in LRU-K cache");
}

template<typename Key, typename Value>
void LruKCache<Key, Value>::put(const Key& key, const Value& value) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    int k_val = k_.load(std::memory_order_acquire);
    int cap = capacity_.load(std::memory_order_acquire);
    
    // 首先检查是否在缓存队列中
    auto cache_it = cache_.find(key);
    if (cache_it != cache_.end()) {
        // 在缓存中存在，更新值并记录访问
        cache_it->second->value = value;
        recordCacheAccess(cache_it->second);
        return;
    }
    
    // 记录历史访问
    recordHistoryAccess(key);
    
    // 检查是否达到K次访问
    auto history_it = history_.find(key);
    if (history_it != history_.end() && 
        history_it->second->access_times.size() >= static_cast<size_t>(k_val)) {
        
        // 达到K次访问，需要提升到缓存队列
        
        // 如果缓存已满，先淘汰一个数据
        if (cache_.size() >= static_cast<size_t>(cap)) {
            Key victim_key = findVictimKey();
            
            // 判断被淘汰的是历史记录还是缓存记录
            auto victim_cache_it = cache_.find(victim_key);
            if (victim_cache_it != cache_.end()) {
                // 淘汰缓存记录
                cache_.erase(victim_cache_it);
            } else {
                // 淘汰历史记录
                history_.erase(victim_key);
            }
        }
        
        // 提升到缓存队列
        promoteToCache(key, value);
    }
    // 如果访问次数不足K次，只保留在历史队列中，不存储实际数据
}

template<typename Key, typename Value>
bool LruKCache<Key, Value>::contains(const Key& key) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    // 只有在缓存队列中的数据才算真正被缓存
    return cache_.find(key) != cache_.end();
}

template<typename Key, typename Value>
void LruKCache<Key, Value>::clear() {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    cache_.clear();
    history_.clear();
}

template<typename Key, typename Value>
std::string LruKCache<Key, Value>::getPolicyName() const {
    int k_val = k_.load(std::memory_order_acquire);
    return "LRU-" + std::to_string(k_val);
}

template<typename Key, typename Value>
int LruKCache<Key, Value>::getHistoryAccessCount(const Key& key) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    auto it = history_.find(key);
    if (it == history_.end()) {
        return 0;
    }
    return static_cast<int>(it->second->access_times.size());
}

template<typename Key, typename Value>
int LruKCache<Key, Value>::getCacheAccessCount(const Key& key) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    auto it = cache_.find(key);
    if (it == cache_.end()) {
        return 0;
    }
    return static_cast<int>(it->second->access_times.size());
}

// 显式实例化模板，支持常用类型
template class LruKCache<int, int>;
template class LruKCache<int, std::string>;
template class LruKCache<std::string, int>;
template class LruKCache<std::string, std::string>;