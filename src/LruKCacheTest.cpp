#include "../include/LruKCache.h"
#include "LruKCache.cpp"
#include <iostream>

int main() {
    try {
        // 创建容量为3，K=2的LRU-K缓存
        LruKCache<int, std::string> cache(3, 2);
        
        std::cout << "=== LRU-K(K=2)算法测试 ===" << std::endl;
        
        // 测试1: 访问次数不足K次的数据不会被缓存
        std::cout << "\n1. 测试访问次数不足K次的数据:" << std::endl;
        cache.put(1, "value1");  // 第1次访问，只在历史队列中
        std::cout << "put(1, value1) - 历史访问次数: " << cache.getHistoryAccessCount(1) << std::endl;
        std::cout << "缓存中是否包含key 1: " << (cache.contains(1) ? "是" : "否") << std::endl;
        
        // 测试2: 第二次访问后，数据被提升到缓存队列
        std::cout << "\n2. 测试第K次访问后数据被提升到缓存:" << std::endl;
        cache.put(1, "value1_updated");  // 第2次访问，达到K次，提升到缓存队列
        std::cout << "put(1, value1_updated) - 历史访问次数: " << cache.getHistoryAccessCount(1) << std::endl;
        std::cout << "缓存中是否包含key 1: " << (cache.contains(1) ? "是" : "否") << std::endl;
        std::cout << "缓存访问次数: " << cache.getCacheAccessCount(1) << std::endl;
        
        // 测试3: get操作只能获取缓存队列中的数据
        std::cout << "\n3. 测试get操作:" << std::endl;
        try {
            std::string value = cache.get(1);
            std::cout << "get(1) = " << value << std::endl;
        } catch (const std::exception& e) {
            std::cout << "get(1)异常: " << e.what() << std::endl;
        }
        
        // 测试4: 添加更多数据测试淘汰机制
        std::cout << "\n4. 测试淘汰机制:" << std::endl;
        cache.put(2, "value2");  // 第1次访问key2
        cache.put(3, "value3");  // 第1次访问key3
        cache.put(4, "value4");  // 第1次访问key4
        
        std::cout << "添加key2,3,4后:" << std::endl;
        std::cout << "历史队列中key2访问次数: " << cache.getHistoryAccessCount(2) << std::endl;
        std::cout << "历史队列中key3访问次数: " << cache.getHistoryAccessCount(3) << std::endl;
        std::cout << "历史队列中key4访问次数: " << cache.getHistoryAccessCount(4) << std::endl;
        
        // 将key2提升到缓存队列
        cache.put(2, "value2_updated");  // 第2次访问key2，提升到缓存
        std::cout << "\nkey2第二次访问后:" << std::endl;
        std::cout << "缓存中包含key2: " << (cache.contains(2) ? "是" : "否") << std::endl;
        
        // 测试缓存大小
        std::cout << "\n5. 缓存状态:" << std::endl;
        std::cout << "当前缓存大小: " << cache.size() << std::endl;
        std::cout << "缓存容量: " << cache.capacity() << std::endl;
        
        std::cout << "\n=== 测试完成 ===" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "测试过程中发生异常: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}