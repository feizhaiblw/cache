#include "../include/LruCache.h"
#include "LruCache.cpp"  // 包含实现文件，因为是模板类
#include <iostream>
#include <cassert>
#include <vector>

void testBasicOperations() {
    std::cout << "=== 测试基本操作 ===" << std::endl;
    
    LruCache<int, std::string> cache(3);
    
    // 测试空缓存
    assert(cache.empty());
    assert(cache.size() == 0);
    assert(cache.capacity() == 3);
    assert(cache.getPolicyName() == "LRU");
    
    // 测试插入
    cache.put(1, "One");
    cache.put(2, "Two");
    cache.put(3, "Three");
    
    assert(cache.size() == 3);
    assert(!cache.empty());
    assert(cache.contains(1));
    assert(cache.contains(2));
    assert(cache.contains(3));
    
    std::cout << "✓ 基本操作测试通过" << std::endl;
}

void testLruEviction() {
    std::cout << "=== 测试LRU淘汰机制 ===" << std::endl;
    
    LruCache<int, std::string> cache(3);
    
    // 填满缓存
    cache.put(1, "One");
    cache.put(2, "Two");
    cache.put(3, "Three");
    
    // 访问键1，使其成为最近使用
    std::string value = cache.get(1);
    assert(value == "One");
    
    // 插入新元素，应该淘汰键2（最久未使用）
    cache.put(4, "Four");
    
    assert(cache.contains(1));  // 最近访问过，应该保留
    assert(!cache.contains(2)); // 最久未使用，应该被淘汰
    assert(cache.contains(3));  // 应该保留
    assert(cache.contains(4));  // 新插入的，应该存在
    
    std::cout << "✓ LRU淘汰机制测试通过" << std::endl;
}

void testUpdateExistingKey() {
    std::cout << "=== 测试更新现有键 ===" << std::endl;
    
    LruCache<int, std::string> cache(2);
    
    cache.put(1, "One");
    cache.put(2, "Two");
    
    // 更新现有键的值
    cache.put(1, "Updated One");
    
    assert(cache.get(1) == "Updated One");
    assert(cache.size() == 2);
    
    // 插入新键，应该淘汰键2（因为键1刚被更新，成为最近使用）
    cache.put(3, "Three");
    
    assert(cache.contains(1));
    assert(!cache.contains(2));
    assert(cache.contains(3));
    
    std::cout << "✓ 更新现有键测试通过" << std::endl;
}

void testAccessPattern() {
    std::cout << "=== 测试访问模式 ===" << std::endl;
    
    LruCache<int, std::string> cache(3);
    
    cache.put(1, "One");
    cache.put(2, "Two");
    cache.put(3, "Three");
    
    // 访问顺序：3, 1, 2
    cache.get(3);
    cache.get(1);
    cache.get(2);
    
    // 插入新元素，应该淘汰最久未被访问的元素
    // 当前访问顺序（从新到旧）：2, 1, 3
    // 所以应该淘汰3
    cache.put(4, "Four");
    
    assert(cache.contains(1));
    assert(cache.contains(2));
    assert(!cache.contains(3)); // 应该被淘汰
    assert(cache.contains(4));
    
    std::cout << "✓ 访问模式测试通过" << std::endl;
}

void testClearOperation() {
    std::cout << "=== 测试清空操作 ===" << std::endl;
    
    LruCache<int, std::string> cache(3);
    
    cache.put(1, "One");
    cache.put(2, "Two");
    cache.put(3, "Three");
    
    assert(cache.size() == 3);
    
    cache.clear();
    
    assert(cache.empty());
    assert(cache.size() == 0);
    assert(!cache.contains(1));
    assert(!cache.contains(2));
    assert(!cache.contains(3));
    
    // 清空后应该能正常使用
    cache.put(10, "Ten");
    assert(cache.contains(10));
    assert(cache.get(10) == "Ten");
    
    std::cout << "✓ 清空操作测试通过" << std::endl;
}

void testExceptions() {
    std::cout << "=== 测试异常处理 ===" << std::endl;
    
    // 测试无效容量异常
    try {
        LruCache<int, int> invalid_cache(0);
        assert(false); // 不应该到达这里
    } catch (const InvalidCapacityException& e) {
        std::cout << "✓ 捕获到预期的容量异常: " << e.what() << std::endl;
    }
    
    // 测试键不存在异常
    LruCache<int, std::string> cache(2);
    cache.put(1, "One");
    
    try {
        cache.get(999); // 不存在的键
        assert(false); // 不应该到达这里
    } catch (const std::out_of_range& e) {
        std::cout << "✓ 捕获到预期的键不存在异常: " << e.what() << std::endl;
    }
    
    std::cout << "✓ 异常处理测试通过" << std::endl;
}

void demonstrateLruBehavior() {
    std::cout << "\n=== LRU缓存行为演示 ===" << std::endl;
    
    LruCache<int, std::string> cache(3);
    
    std::cout << "容量: " << cache.capacity() << std::endl;
    
    // 步骤1：填满缓存
    std::cout << "\n1. 填满缓存:" << std::endl;
    cache.put(1, "One");
    std::cout << "   插入 (1, One), 大小: " << cache.size() << std::endl;
    cache.put(2, "Two");
    std::cout << "   插入 (2, Two), 大小: " << cache.size() << std::endl;
    cache.put(3, "Three");
    std::cout << "   插入 (3, Three), 大小: " << cache.size() << std::endl;
    
    // 步骤2：访问元素
    std::cout << "\n2. 访问元素:" << std::endl;
    std::cout << "   访问键1: " << cache.get(1) << std::endl;
    
    // 步骤3：插入新元素触发LRU淘汰
    std::cout << "\n3. 插入新元素 (4, Four):" << std::endl;
    cache.put(4, "Four");
    std::cout << "   大小: " << cache.size() << std::endl;
    
    // 步骤4：检查哪些元素被保留
    std::cout << "\n4. 检查缓存内容:" << std::endl;
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
        std::cout << "开始LRU缓存测试..." << std::endl;
        
        testBasicOperations();
        testLruEviction();
        testUpdateExistingKey();
        testAccessPattern();
        testClearOperation();
        testExceptions();
        
        demonstrateLruBehavior();
        
        std::cout << "\n🎉 所有测试通过！LRU缓存实现正确。" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ 测试失败: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}