#!/bin/bash

# 缓存策略多线程测试脚本
# 为所有缓存策略提供严谨的多线程测试

echo "========================================"
echo "缓存策略多线程安全测试套件"
echo "========================================"
echo ""

# 检查构建目录
if [ ! -d "build" ]; then
    echo "构建目录不存在，正在创建..."
    mkdir -p build
    cd build
    cmake ..
    make -j4
    cd ..
else
    echo "使用现有构建目录"
fi

cd build

# 测试结果汇总
declare -A test_results

echo "开始执行多线程测试..."
echo ""

# 测试 LRU 缓存
echo "🔄 正在测试 LRU 缓存多线程安全性..."
if ./LruCacheThreadTest > lru_thread_test.log 2>&1; then
    echo "✅ LRU 缓存多线程测试通过"
    test_results["LRU"]="PASS"
else
    echo "❌ LRU 缓存多线程测试失败"
    test_results["LRU"]="FAIL"
    echo "详细日志请查看: build/lru_thread_test.log"
fi
echo ""

# 测试 FIFO 缓存
echo "🔄 正在测试 FIFO 缓存多线程安全性..."
if ./FifoCacheThreadTest > fifo_thread_test.log 2>&1; then
    echo "✅ FIFO 缓存多线程测试通过"
    test_results["FIFO"]="PASS"
else
    echo "❌ FIFO 缓存多线程测试失败"
    test_results["FIFO"]="FAIL"
    echo "详细日志请查看: build/fifo_thread_test.log"
fi
echo ""

# 测试 LFU 缓存
echo "🔄 正在测试 LFU 缓存多线程安全性..."
if ./LfuCacheThreadTest > lfu_thread_test.log 2>&1; then
    echo "✅ LFU 缓存多线程测试通过"
    test_results["LFU"]="PASS"
else
    echo "❌ LFU 缓存多线程测试失败"
    test_results["LFU"]="FAIL"
    echo "详细日志请查看: build/lfu_thread_test.log"
fi
echo ""

# 测试 LRU-K 缓存
echo "🔄 正在测试 LRU-K 缓存多线程安全性..."
if ./LruKCacheThreadTest > lruk_thread_test.log 2>&1; then
    echo "✅ LRU-K 缓存多线程测试通过"
    test_results["LRU-K"]="PASS"
else
    echo "❌ LRU-K 缓存多线程测试失败"
    test_results["LRU-K"]="FAIL"
    echo "详细日志请查看: build/lruk_thread_test.log"
fi
echo ""

# 汇总测试结果
echo "========================================"
echo "多线程测试结果汇总"
echo "========================================"

pass_count=0
fail_count=0

for strategy in "LRU" "FIFO" "LFU" "LRU-K"; do
    result=${test_results[$strategy]}
    if [ "$result" = "PASS" ]; then
        echo "✅ $strategy 缓存: 通过"
        ((pass_count++))
    else
        echo "❌ $strategy 缓存: 失败"
        ((fail_count++))
    fi
done

echo ""
echo "通过: $pass_count 个策略"
echo "失败: $fail_count 个策略"

if [ $fail_count -eq 0 ]; then
    echo ""
    echo "🎉 所有缓存策略的多线程测试均通过！"
    echo ""
    echo "测试验证了以下重要特性："
    echo "• 数据一致性：在并发访问下数据保持一致"
    echo "• 线程安全：使用外部同步措施确保线程安全"
    echo "• 异常安全：并发环境下的异常处理正确"
    echo "• 性能表现：高并发下的操作性能良好"
    echo "• 策略正确性：各种淘汰策略在并发下仍然正确"
    echo ""
    echo "注意事项："
    echo "• LRU、FIFO、LFU 缓存默认不是线程安全的，需要外部同步"
    echo "• LRU-K 缓存内部使用 shared_mutex，已支持线程安全"
    echo "• 在生产环境中使用时，请根据项目规范添加适当的同步措施"
    
    exit 0
else
    echo ""
    echo "⚠️  有 $fail_count 个策略的多线程测试失败"
    echo "请检查相应的日志文件以获取详细信息"
    
    exit 1
fi