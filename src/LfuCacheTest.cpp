#include "../include/LfuCache.h"
#include "LfuCache.cpp"  // 包含实现文件，因为是模板类
#include <iostream>
#include <cassert>
#include <vector>

void testLfuBasicOperations() {
    std::cout << "=== 测试LFU基本操作 ===" << std::endl;
    
    LfuCache<int, std::string> cache(3);
    
    // 测试空缓存
    assert(cache.empty());
    assert(cache.size() == 0);
    assert(cache.capacity() == 3);
    assert(cache.getPolicyName() == "LFU");
    assert(cache.getMinFrequency() == 1);
    
    // 测试插入
    cache.put(1, "One");
    cache.put(2, "Two");
    cache.put(3, "Three");
    
    assert(cache.size() == 3);
    assert(!cache.empty());
    assert(cache.contains(1));
    assert(cache.contains(2));
    assert(cache.contains(3));
    
    // 测试初始频率
    assert(cache.getFrequency(1) == 1);
    assert(cache.getFrequency(2) == 1);
    assert(cache.getFrequency(3) == 1);
    
    std::cout << "✓ LFU基本操作测试通过" << std::endl;
}

void testLfuFrequencyTracking() {
    std::cout << "=== 测试LFU频率跟踪 ===" << std::endl;
    
    LfuCache<int, std::string> cache(3);
    
    cache.put(1, "One");
    cache.put(2, "Two");
    cache.put(3, "Three");
    
    // 多次访问某些元素
    cache.get(1);  // 频率变为2
    cache.get(1);  // 频率变为3
    cache.get(2);  // 频率变为2
    
    assert(cache.getFrequency(1) == 3);
    assert(cache.getFrequency(2) == 2);
    assert(cache.getFrequency(3) == 1);
    assert(cache.getMinFrequency() == 1);
    
    std::cout << "✓ LFU频率跟踪测试通过" << std::endl;
}

void testLfuEvictionStrategy() {
    std::cout << "=== 测试LFU淘汰策略 ===" << std::endl;
    
    LfuCache<int, std::string> cache(3);
    
    // 插入3个元素
    cache.put(1, "One");
    cache.put(2, "Two");
    cache.put(3, "Three");
    
    // 建立不同的访问频率
    cache.get(1);  // 1的频率为2
    cache.get(1);  // 1的频率为3
    cache.get(2);  // 2的频率为2
    // 3的频率仍为1
    
    // 插入新元素，应该淘汰频率最低的元素3
    cache.put(4, "Four");
    
    assert(cache.contains(1));  // 频率3，应该保留
    assert(cache.contains(2));  // 频率2，应该保留
    assert(!cache.contains(3)); // 频率1，应该被淘汰
    assert(cache.contains(4));  // 新插入的，应该存在
    
    std::cout << "✓ LFU淘汰策略测试通过" << std::endl;
}

void testLfuTieBreaking() {
    std::cout << "=== 测试LFU相同频率时的LRU策略 ===" << std::endl;
    
    LfuCache<int, std::string> cache(3);
    
    cache.put(1, "One");   // 频率1
    cache.put(2, "Two");   // 频率1
    cache.put(3, "Three"); // 频率1
    
    // 所有元素频率相同，应该按LRU策略淘汰最久未使用的
    // 此时1是最早插入的
    
    cache.put(4, "Four");
    
    assert(!cache.contains(1)); // 最早插入且频率相同，应该被淘汰
    assert(cache.contains(2));
    assert(cache.contains(3));
    assert(cache.contains(4));
    
    std::cout << "✓ LFU相同频率时的LRU策略测试通过" << std::endl;
}

void testLfuUpdateExistingKey() {
    std::cout << "=== 测试LFU更新现有键 ===" << std::endl;
    
    LfuCache<int, std::string> cache(2);
    
    cache.put(1, "One");
    cache.put(2, "Two");
    
    // 更新现有键的值，put操作会增加频率
    cache.put(1, "Updated One");
    
    assert(cache.get(1) == "Updated One");
    assert(cache.getFrequency(1) == 3); // put操作(+1) + get操作(+1) = 3
    assert(cache.size() == 2);
    
    // 插入新键，应该淘汰频率较低的键2
    cache.put(3, "Three");
    
    assert(cache.contains(1));  // 频率3，应该保留
    assert(!cache.contains(2)); // 频率1，应该被淘汰
    assert(cache.contains(3));  // 新插入的
    
    std::cout << "✓ LFU更新现有键测试通过" << std::endl;
}

void testLfuComplexScenario() {
    std::cout << "=== 测试LFU复杂场景 ===" << std::endl;
    
    LfuCache<int, std::string> cache(4);
    
    // 创建复杂的频率分布
    cache.put(1, "One");   // 频率1
    cache.put(2, "Two");   // 频率1
    cache.put(3, "Three"); // 频率1
    cache.put(4, "Four");  // 频率1
    
    // 建立不同频率
    cache.get(4);  // 4的频率为2
    cache.get(3);  // 3的频率为2
    cache.get(4);  // 4的频率为3
    cache.get(2);  // 2的频率为2
    // 现在频率分布：4(3), 3(2), 2(2), 1(1)
    
    // 插入新元素，应该淘汰频率最低的1
    cache.put(5, "Five");
    
    assert(!cache.contains(1)); // 频率1，应该被淘汰
    assert(cache.contains(2));  // 频率2，应该保留
    assert(cache.contains(3));  // 频率2，应该保留
    assert(cache.contains(4));  // 频率3，应该保留
    assert(cache.contains(5));  // 新插入的
    
    // 再次插入，现在最小频率是1（新元素5）
    cache.put(6, "Six");
    
    assert(!cache.contains(5)); // 频率1且是最新的最小频率，应该被淘汰
    assert(cache.contains(6));
    
    std::cout << "✓ LFU复杂场景测试通过" << std::endl;
}

void testLfuClearOperation() {
    std::cout << "=== 测试LFU清空操作 ===" << std::endl;
    
    LfuCache<int, std::string> cache(3);
    
    cache.put(1, "One");
    cache.put(2, "Two");
    cache.put(3, "Three");
    cache.get(1); // 增加频率
    
    assert(cache.size() == 3);
    assert(cache.getFrequency(1) == 2);
    
    cache.clear();
    
    assert(cache.empty());
    assert(cache.size() == 0);
    assert(!cache.contains(1));
    assert(!cache.contains(2));
    assert(!cache.contains(3));
    assert(cache.getMinFrequency() == 1);
    
    // 清空后应该能正常使用
    cache.put(10, "Ten");
    assert(cache.contains(10));
    assert(cache.get(10) == "Ten");
    assert(cache.getFrequency(10) == 2);
    
    std::cout << "✓ LFU清空操作测试通过" << std::endl;
}

void testLfuFrequencyJump() {
    std::cout << "=== 测试LFU频率跳跃时的最小频率更新 ===" << std::endl;
    
    // 测试场景1：正常的频率递增（优化路径）
    {
        LfuCache<int, std::string> cache(2);
        cache.put(1, "One");    // 频率: 1
        cache.put(2, "Two");    // 频率: 1
        
        // 将键1的频率从1提升到2（递增）
        cache.get(1);
        
        std::cout << "   场景1 - 递增频率更新:" << std::endl;
        std::cout << "     键1频率: " << cache.getFrequency(1) << ", 键2频率: " << cache.getFrequency(2) << std::endl;
        std::cout << "     最小频率: " << cache.getMinFrequency() << std::endl;
        
        // 再次访问键2，频率从1变为2
        cache.get(2);
        
        std::cout << "     更新后最小频率: " << cache.getMinFrequency() << std::endl;
        
        if (cache.getMinFrequency() == 2) {
            std::cout << "     ✓ 递增场景测试通过" << std::endl;
        }
    }
    
    std::cout << "✓ LFU频率跳跃测试完成" << std::endl;
}

void testLfuExceptions() {
    std::cout << "=== 测试LFU异常处理 ===" << std::endl;
    
    // 测试无效容量异常
    try {
        LfuCache<int, int> invalid_cache(0);
        assert(false); // 不应该到达这里
    } catch (const InvalidCapacityException& e) {
        std::cout << "✓ 捕获到预期的容量异常: " << e.what() << std::endl;
    }
    
    // 测试键不存在异常
    LfuCache<int, std::string> cache(2);
    cache.put(1, "One");
    
    try {
        cache.get(999); // 不存在的键
        assert(false); // 不应该到达这里
    } catch (const std::out_of_range& e) {
        std::cout << "✓ 捕获到预期的键不存在异常: " << e.what() << std::endl;
    }
    
    // 测试零容量缓存
    try {
        LfuCache<int, std::string> zero_cache(0);
        assert(false); // 不应该到达这里
    } catch (const InvalidCapacityException& e) {
        std::cout << "✓ 捕获到预期的零容量异常: " << e.what() << std::endl;
    }
    
    std::cout << "✓ LFU异常处理测试通过" << std::endl;
}

void demonstrateLfuBehavior() {
    std::cout << "\n=== LFU缓存行为演示 ===" << std::endl;
    
    LfuCache<int, std::string> cache(3);
    
    std::cout << "容量: " << cache.capacity() << std::endl;
    
    // 步骤1：填满缓存
    std::cout << "\n1. 填满缓存:" << std::endl;
    cache.put(1, "First");
    std::cout << "   插入 (1, First), 频率: " << cache.getFrequency(1) << std::endl;
    cache.put(2, "Second");
    std::cout << "   插入 (2, Second), 频率: " << cache.getFrequency(2) << std::endl;
    cache.put(3, "Third");
    std::cout << "   插入 (3, Third), 频率: " << cache.getFrequency(3) << std::endl;
    
    // 步骤2：建立访问频率差异
    std::cout << "\n2. 建立访问频率差异:" << std::endl;
    cache.get(1);
    std::cout << "   访问键1, 频率: " << cache.getFrequency(1) << std::endl;
    cache.get(1);
    std::cout << "   再次访问键1, 频率: " << cache.getFrequency(1) << std::endl;
    cache.get(2);
    std::cout << "   访问键2, 频率: " << cache.getFrequency(2) << std::endl;
    
    std::cout << "   当前最小频率: " << cache.getMinFrequency() << std::endl;
    
    // 步骤3：插入新元素触发LFU淘汰
    std::cout << "\n3. 插入新元素 (4, Fourth):" << std::endl;
    cache.put(4, "Fourth");
    std::cout << "   大小: " << cache.size() << std::endl;
    
    // 步骤4：检查哪些元素被保留
    std::cout << "\n4. 检查缓存内容（LFU策略）:" << std::endl;
    std::vector<int> keys = {1, 2, 3, 4};
    for (int key : keys) {
        if (cache.contains(key)) {
            std::cout << "   键" << key << ": " << cache.get(key) 
                      << " (存在, 频率: " << cache.getFrequency(key) << ")" << std::endl;
        } else {
            std::cout << "   键" << key << ": (已被淘汰)" << std::endl;
        }
    }
    
    std::cout << "\n   分析：键3因为访问频率最低(1次)而被淘汰" << std::endl;
}

int main() {
    try {
        std::cout << "开始LFU缓存测试..." << std::endl;
        
        testLfuBasicOperations();
        testLfuFrequencyTracking();
        testLfuEvictionStrategy();
        testLfuTieBreaking();
        testLfuUpdateExistingKey();
        testLfuComplexScenario();
        testLfuClearOperation();
        testLfuFrequencyJump();
        testLfuExceptions();
        
        demonstrateLfuBehavior();
        
        std::cout << "\n🎉 所有LFU测试通过！LFU缓存实现正确。" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ 测试失败: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}