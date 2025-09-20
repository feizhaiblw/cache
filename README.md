# 缓存策略框架

这是一个高性能、可扩展的C++缓存策略框架，支持多种缓存淘汰算法和线程安全控制。

## 🚀 项目特点

- **多策略支持**: 实现了LRU、FIFO、LFU、LRU-K四种主流缓存策略
- **统一接口**: 所有缓存策略继承统一的抽象接口，便于切换和扩展
- **线程安全**: 使用`shared_mutex`读写锁实现高效的并发控制
- **现代C++**: 采用C++17标准，利用智能指针、RAII、移动语义等特性
- **类型安全**: 模板化设计，支持任意类型的键值对
- **异常安全**: 完整的异常处理机制，保证操作的原子性
- **高性能**: 所有核心操作时间复杂度为O(1)
- **完整测试**: 包含单元测试和多线程安全测试

## 📁 项目结构

```
Cache/
├── include/                    # 头文件目录
│   ├── CachePolicy.h             # 缓存策略抽象接口
│   ├── LruCache.h                # LRU缓存头文件
│   ├── FifoCache.h               # FIFO缓存头文件
│   ├── LfuCache.h                # LFU缓存头文件
│   ├── LruKCache.h               # LRU-K缓存头文件
│   └── ThreadSafeTestFramework.h # 多线程测试框架
├── src/                        # 源文件目录
│   ├── LruCache.cpp              # LRU缓存实现
│   ├── FifoCache.cpp             # FIFO缓存实现
│   ├── LfuCache.cpp              # LFU缓存实现
│   ├── LruKCache.cpp             # LRU-K缓存实现
│   ├── *Test.cpp                 # 单元测试文件
│   ├── *ThreadTest.cpp           # 多线程测试文件
│   ├── CacheExample.cpp          # 使用示例
│   └── CacheComparison.cpp       # 性能对比示例
├── build/                      # 编译输出目录
├── Makefile                    # GNU Make构建配置
├── CMakeLists.txt              # CMake构建配置
└── README.md                   # 项目说明
```

## 🏗️ 架构设计

### 1. 抽象接口 (`include/CachePolicy.h`)
- 定义了所有缓存策略必须实现的统一接口
- 支持泛型设计，可以处理任意类型的键值对
- 包含完整的异常处理机制

### 2. 具体实现
- **LRU Cache**: 最近最少使用算法，使用双向链表+哈希表实现
- **FIFO Cache**: 先进先出算法，使用队列+哈希表实现
- **LFU Cache**: 最少使用频率算法，使用频率桶+双向链表实现
- **LRU-K Cache**: LRU的K距离变种，维护访问历史记录

### 3. 线程安全模型
- **LRU、FIFO、LFU**: 外部同步模型，使用`std::shared_mutex`读写锁
- **LRU-K**: 内部同步模型，内置线程安全机制
- 读操作支持并发，写操作独占访问

### 4. 设计模式
- **策略模式**: 不同缓存算法实现统一接口
- **模板模式**: 泛型支持任意键值类型
- **RAII模式**: 自动资源管理

## ✨ 主要特性

### 🔧 统一接口
所有缓存策略都实现相同的接口，便于切换和测试：
```cpp
virtual Value get(const Key& key) = 0;
virtual void put(const Key& key, const Value& value) = 0;
virtual bool contains(const Key& key) const = 0;
virtual int size() const = 0;
virtual int capacity() const = 0;
virtual bool empty() const = 0;
virtual void clear() = 0;
virtual std::string getPolicyName() const = 0;
```

### 🔒 线程安全支持
所有缓存策略均实现了内部线程安全：
```cpp
// 直接使用，无需外部同步
LruCache<int, std::string> lru_cache(100);
FifoCache<int, std::string> fifo_cache(100);
LfuCache<int, std::string> lfu_cache(100);
LruKCache<int, std::string> lruk_cache(100, 2);

void thread_safe_example() {
    // 直接使用，内部已实现线程安全
    lru_cache.put(1, "Hello");
    lru_cache.put(2, "World");
    
    if (lru_cache.contains(1)) {
        std::string value = lru_cache.get(1);
        std::cout << "Value: " << value << std::endl;
    }
    
    std::cout << "Cache type: " << lru_cache.getPolicyName() << std::endl;
    std::cout << "Size: " << lru_cache.size() << "/" << lru_cache.capacity() << std::endl;
}
```

### 💾 内部线程安全实现
所有缓存策略都使用`std::shared_mutex`实现高效的读写锁：
- **读操作** (`get`, `contains`, `size`, `empty`)：支持并发访问
- **写操作** (`put`, `clear`)：独占访问，保证数据一致性

### 💾 类型安全
- 使用C++模板提供编译时类型检查
- 支持任意类型的键值对

### ⚡ 异常安全
- 定义了专门的异常类型
- 提供详细的错误信息
- 保证操作的原子性

### 🚀 现代C++特性
- 使用智能指针管理内存
- 支持移动语义
- RAII原则
- 禁用不必要的拷贝操作

### 📈 可扩展性
- 易于添加新的缓存策略
- 遵循开闭原则（对扩展开放，对修改关闭）

## 💡 使用示例

### 基础使用
```cpp
#include "CachePolicy.h"
#include "LruCache.h"
#include "FifoCache.h"
#include "LfuCache.h"
#include "LruKCache.h"
#include <shared_mutex>

// 创建不同类型的缓存
LruCache<int, std::string> lru_cache(100);
FifoCache<int, std::string> fifo_cache(100);
LfuCache<int, std::string> lfu_cache(100);
LruKCache<int, std::string> lruk_cache(100, 2);  // 容量100，K=2

// 直接使用（内部已实现线程安全）
void simple_usage_example() {
    // 直接使用，无需额外加锁
    lru_cache.put(1, "Hello");
    lru_cache.put(2, "World");
    
    if (lru_cache.contains(1)) {
        std::string value = lru_cache.get(1);
        std::cout << "Value: " << value << std::endl;
    }
    
    std::cout << "Cache type: " << lru_cache.getPolicyName() << std::endl;
    std::cout << "Size: " << lru_cache.size() << "/" << lru_cache.capacity() << std::endl;
}
```

### 策略对比
```cpp
void compare_strategies() {
    const int capacity = 3;
    
    LruCache<int, std::string> lru(capacity);
    FifoCache<int, std::string> fifo(capacity);
    LfuCache<int, std::string> lfu(capacity);
    
    // 填充缓存
    for (int i = 1; i <= 4; ++i) {
        lru.put(i, "value" + std::to_string(i));
        fifo.put(i, "value" + std::to_string(i));
        lfu.put(i, "value" + std::to_string(i));
    }
    
    // LRU: 保留最近访问的数据 [2,3,4]
    // FIFO: 保留最后插入的数据 [2,3,4]
    // LFU: 保留访问频率最高的数据（新插入数据频率为1）
}
```

## 📊 支持的缓存策略

| 策略 | 文件 | 描述 | 时间复杂度 | 线程安全 | 适用场景 |
|------|------|------|------------|----------|----------|
| **LRU** | `LruCache.h` | 最近最少使用 | O(1) | 内部同步 | 时间局部性强的场景 |
| **FIFO** | `FifoCache.h` | 先进先出 | O(1) | 内部同步 | 简单队列缓存 |
| **LFU** | `LfuCache.h` | 最少使用频率 | O(1) | 内部同步 | 频率局部性强的场景 |
| **LRU-K** | `LruKCache.h` | LRU的K距离变种 | O(1) | 内部同步 | 避免缓存污染 |

### 策略特点详解

#### 🔄 LRU (Least Recently Used)
- **原理**: 淘汰最久未被访问的数据
- **实现**: 双向链表 + 哈希表
- **特点**: 基于时间局部性原理，适合大多数场景

#### 📥 FIFO (First In First Out) 
- **原理**: 淘汰最早插入的数据，不考虑访问频率
- **实现**: 队列 + 哈希表
- **特点**: 简单直观，适合流式数据处理

#### 📈 LFU (Least Frequently Used)
- **原理**: 淘汰访问频率最低的数据，频率相同时使用LRU策略
- **实现**: 频率桶 + 双向链表 + 哈希表
- **特点**: 基于频率局部性，适合热点数据明显的场景

#### 🎯 LRU-K
- **原理**: 只有被访问K次以上的数据才进入缓存队列
- **实现**: 双队列设计（历史队列 + 缓存队列）
- **特点**: 有效避免缓存污染，适合有突发访问的场景

## 🔨 编译和运行

### 系统要求
- **编译器**: 支持C++17的g++ (GCC 7+)
- **构建工具**: GNU Make 或 CMake 3.10+
- **依赖**: pthread库

### 使用Makefile构建
```bash
# 编译所有目标
make all

# 编译基础测试
make tests

# 编译多线程测试
make thread_tests

# 编译示例程序
make examples

# 运行所有基础测试
make test

# 运行多线程测试
make thread_test

# 运行缓存示例
make demo

# 运行性能对比
make comparison

# 清理编译文件
make clean

# 显示帮助信息
make help
```

### 使用CMake构建
```bash
# 创建构建目录
mkdir build && cd build

# 配置项目
cmake ..

# 编译所有目标
make

# 运行基础测试
make run_tests

# 运行多线程测试
make run_thread_tests

# 使用CTest运行测试
ctest --verbose
```

### 单独编译
```bash
# 编译LRU缓存测试
g++ -std=c++17 -Wall -Wextra -O2 -Iinclude src/LruCacheTest.cpp -o lru_test

# 编译多线程测试（需要pthread）
g++ -std=c++17 -Wall -Wextra -O2 -Iinclude src/LruCacheThreadTest.cpp -o lru_thread_test -lpthread
```

## ⚠️ 注意事项

### 线程安全
| 策略 | 线程安全模式 | 使用方式 |
|------|-------------|----------|
| LRU、FIFO、LFU、LRU-K | **内部同步** | 内部已实现`std::shared_mutex`线程安全，可直接使用 |


### 内存管理
- 使用RAII原则，自动管理内存，无需手动释放
- 禁用拷贝构造和拷贝赋值，避免意外的深拷贝
- 支持移动语义，提高性能

### 异常处理
- 所有接口都可能抛出异常，请妥善处理
- 主要异常类型：
  - `InvalidCapacityException`: 容量参数无效
  - `std::out_of_range`: 键不存在

### 容量限制
- 缓存容量必须大于0，否则会抛出`InvalidCapacityException`
- LRU-K缓存的K值必须大于0

### 性能考虑
- 所有核心操作（get、put、contains）时间复杂度为O(1)
- 多线程环境下，读操作支持并发，写操作需要独占访问
- LFU缓存在高频率访问场景下内存占用可能较高

## 🔧 扩展新策略

要添加新的缓存策略，只需要以下步骤：

### 1. 创建头文件
```cpp
// include/NewCache.h
#ifndef NEW_CACHE_H
#define NEW_CACHE_H

#include "CachePolicy.h"
#include <shared_mutex>

template<typename Key, typename Value>
class NewCache : public CachePolicy<Key, Value> {
private:
    int capacity_;
    // 你的数据结构
    mutable std::shared_mutex mutex_;  // 如需外部同步
    
public:
    explicit NewCache(int capacity);
    ~NewCache() override = default;
    
    // 实现所有纯虚函数
    Value get(const Key& key) override;
    void put(const Key& key, const Value& value) override;
    bool contains(const Key& key) const override;
    int size() const override;
    int capacity() const override;
    bool empty() const override;
    void clear() override;
    std::string getPolicyName() const override;
};

#endif
```

### 2. 实现源文件
```cpp
// src/NewCache.cpp
#include "../include/NewCache.h"
#include <mutex>

template<typename Key, typename Value>
NewCache<Key, Value>::NewCache(int capacity) : capacity_(capacity) {
    if (capacity <= 0) {
        throw InvalidCapacityException(capacity);
    }
    // 初始化你的数据结构
}

// 实现所有方法...

// 显式实例化
template class NewCache<int, int>;
template class NewCache<int, std::string>;
template class NewCache<std::string, int>;
template class NewCache<std::string, std::string>;
```

### 3. 添加测试
```cpp
// src/NewCacheTest.cpp
#include "../include/NewCache.h"
#include <iostream>
#include <cassert>

void testNewCacheBasicOperations() {
    NewCache<int, std::string> cache(3);
    
    // 添加你的测试用例
    cache.put(1, "one");
    assert(cache.get(1) == "one");
    assert(cache.size() == 1);
    
    std::cout << "✓ NewCache basic operations test passed" << std::endl;
}

int main() {
    testNewCacheBasicOperations();
    std::cout << "🎉 All NewCache tests passed!" << std::endl;
    return 0;
}
```

### 4. 更新构建配置
在Makefile中添加新的目标：
```makefile
new_test: $(SRC_DIR)/NewCacheTest.cpp $(INC_DIR)/NewCache.h $(SRC_DIR)/NewCache.cpp
	$(CXX) $(CXXFLAGS) $(SRC_DIR)/NewCacheTest.cpp -o $(BUILD_DIR)/new_test
```

这样的设计确保了代码的可维护性和可扩展性。

## 🧪 测试覆盖

### 基础功能测试
- ✅ 基本操作测试（插入、查询、更新、删除）
- ✅ 容量限制和淘汰机制测试
- ✅ 边界条件测试（空缓存、单元素、满缓存）
- ✅ 异常处理测试（无效容量、键不存在）
- ✅ 策略特性测试（LRU顺序、FIFO顺序、LFU频率、LRU-K历史）

### 多线程安全测试
- ✅ 并发插入操作测试
- ✅ 并发读取操作测试（读写锁验证）
- ✅ 混合读写操作测试
- ✅ 数据一致性验证
- ✅ 异常安全性测试
- ✅ 高并发压力测试

### 性能基准测试
典型性能表现（混合操作）：
- **LRU**: ~2,600,000 ops/sec
- **FIFO**: ~2,400,000 ops/sec 
- **LFU**: ~2,400,000 ops/sec
- **LRU-K**: ~1,000,000 ops/sec

## 📚 相关文档

- [`MULTITHREAD_TESTING.md`](MULTITHREAD_TESTING.md) - 多线程安全测试详细说明
- [`run_multithread_tests.sh`](run_multithread_tests.sh) - 多线程测试执行脚本
- API文档请参考各头文件中的详细注释

## 🤝 贡献指南

1. **代码规范**: 遵循现代C++最佳实践
2. **测试要求**: 新功能必须包含完整的单元测试和多线程测试
3. **文档更新**: 修改功能时请同步更新相关文档
4. **性能要求**: 核心操作时间复杂度必须为O(1)

## 📄 许可证

本项目采用开源许可证，具体请查看LICENSE文件。

## 🙏 致谢

感谢所有贡献者对本项目的支持和改进！

---

**快速开始**：`make all && make test && make thread_test`