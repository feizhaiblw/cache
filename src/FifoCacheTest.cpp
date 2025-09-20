#include "../include/FifoCache.h"
#include "FifoCache.cpp"  // 包含实现文件，因为是模板类
#include <iostream>
#include <cassert>
#include <vector>

void testFifoBasicOperations() {
    std::cout << "=== 测试FIFO基本操作 ===" << std::endl;
    
    FifoCache<int, std::string> cache(3);
    
    // 测试空缓存
    assert(cache.empty());
    assert(cache.size() == 0);
    assert(cache.capacity() == 3);
    assert(cache.getPolicyName() == "FIFO");
    
    // 测试插入
    cache.put(1, "One");
    cache.put(2, "Two");
    cache.put(3, "Three");
    
    assert(cache.size() == 3);
    assert(!cache.empty());
    assert(cache.contains(1));
    assert(cache.contains(2));
    assert(cache.contains(3));
    
    std::cout << "✓ FIFO基本操作测试通过" << std::endl;
}

void testFifoEviction() {
    std::cout << "=== 测试FIFO淘汰机制 ===" << std::endl;
    
    FifoCache<int, std::string> cache(3);
    
    // 填满缓存，插入顺序：1 -> 2 -> 3
    cache.put(1, "One");
    cache.put(2, "Two");
    cache.put(3, "Three");
    
    // 访问键1（FIFO不受访问影响，仍按插入顺序淘汰）
    std::string value = cache.get(1);
    assert(value == "One");
    
    // 插入新元素，应该淘汰键1（最早插入的）
    cache.put(4, "Four");
    
    assert(!cache.contains(1)); // 最早插入，应该被淘汰
    assert(cache.contains(2));  // 应该保留
    assert(cache.contains(3));  // 应该保留
    assert(cache.contains(4));  // 新插入的，应该存在
    
    std::cout << "✓ FIFO淘汰机制测试通过" << std::endl;
}

void testFifoUpdateExistingKey() {
    std::cout << "=== 测试FIFO更新现有键 ===" << std::endl;
    
    FifoCache<int, std::string> cache(2);
    
    cache.put(1, "One");
    cache.put(2, "Two");
    
    // 更新现有键的值（不改变插入顺序）
    cache.put(1, "Updated One");
    
    assert(cache.get(1) == "Updated One");
    assert(cache.size() == 2);
    
    // 插入新键，应该淘汰键1（最早插入的，虽然被更新过但插入顺序不变）
    cache.put(3, "Three");
    
    assert(!cache.contains(1)); // 应该被淘汰
    assert(cache.contains(2));
    assert(cache.contains(3));
    
    std::cout << "✓ FIFO更新现有键测试通过" << std::endl;
}

void testFifoVsLru() {
    std::cout << "=== 测试FIFO与LRU的差异 ===" << std::endl;
    
    FifoCache<int, std::string> fifo(3);
    
    // 插入顺序：1 -> 2 -> 3
    fifo.put(1, "One");
    fifo.put(2, "Two");
    fifo.put(3, "Three");
    
    // 多次访问键1（在FIFO中不影响淘汰顺序）
    fifo.get(1);
    fifo.get(1);
    fifo.get(1);
    
    // 插入新元素，FIFO仍然淘汰键1（最早插入的）
    fifo.put(4, "Four");
    
    assert(!fifo.contains(1)); // FIFO：最早插入的被淘汰，不受访问影响
    assert(fifo.contains(2));
    assert(fifo.contains(3));
    assert(fifo.contains(4));
    
    std::cout << "✓ FIFO与LRU差异测试通过" << std::endl;
}

void demonstrateFifoBehavior() {
    std::cout << "\n=== FIFO缓存行为演示 ===" << std::endl;
    
    FifoCache<int, std::string> cache(3);
    
    std::cout << "容量: " << cache.capacity() << std::endl;
    
    // 步骤1：填满缓存
    std::cout << "\n1. 按顺序填满缓存:" << std::endl;
    cache.put(1, "First");
    std::cout << "   插入 (1, First), 大小: " << cache.size() << std::endl;
    cache.put(2, "Second");
    std::cout << "   插入 (2, Second), 大小: " << cache.size() << std::endl;
    cache.put(3, "Third");
    std::cout << "   插入 (3, Third), 大小: " << cache.size() << std::endl;
    
    // 步骤2：访问元素（不影响FIFO顺序）
    std::cout << "\n2. 访问最早的元素:" << std::endl;
    std::cout << "   访问键1: " << cache.get(1) << " (这不会影响FIFO淘汰顺序)" << std::endl;
    
    // 步骤3：插入新元素触发FIFO淘汰
    std::cout << "\n3. 插入新元素 (4, Fourth):" << std::endl;
    cache.put(4, "Fourth");
    std::cout << "   大小: " << cache.size() << std::endl;
    
    // 步骤4：检查哪些元素被保留
    std::cout << "\n4. 检查缓存内容（按先进先出原则淘汰）:" << std::endl;
    std::vector<int> keys = {1, 2, 3, 4};
    for (int key : keys) {
        if (cache.contains(key)) {
            std::cout << "   键" << key << ": " << cache.get(key) << " (存在)" << std::endl;
        } else {
            std::cout << "   键" << key << ": (已被淘汰)" << std::endl;
        }
    }
}

int main() {
    try {
        std::cout << "开始FIFO缓存测试..." << std::endl;
        
        testFifoBasicOperations();
        testFifoEviction();
        testFifoUpdateExistingKey();
        testFifoVsLru();
        
        demonstrateFifoBehavior();
        
        std::cout << "\n🎉 所有FIFO测试通过！FIFO缓存实现正确。" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ 测试失败: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}