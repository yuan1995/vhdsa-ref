#!/bin/bash

# 性能测试脚本 - 运行API测试100次并计算平均时间
echo "VHDSA Performance Benchmark - 10 iterations"
echo "=============================================="

# 测试程序路径
VHDSA_192F="build/vhdsa_192f/vhdsa_192f_api_test"
VHDSA_256F="build/vhdsa_256f/vhdsa_256f_api_test"

# 检查程序是否存在
if [ ! -f "$VHDSA_192F" ]; then
    echo "Error: $VHDSA_192F not found!"
    exit 1
fi

if [ ! -f "$VHDSA_256F" ]; then
    echo "Error: $VHDSA_256F not found!"
    exit 1
fi

# 检查bc命令是否可用
if ! command -v bc &> /dev/null; then
    echo "Error: bc command not found! Please install bc: sudo apt-get install bc"
    exit 1
fi

# 函数：运行测试并提取时间
run_test() {
    local test_program=$1
    local test_name=$2
    local iterations=10
    
    echo ""
    echo "Testing $test_name ($iterations iterations)..."
    echo "------------------------------------------------"
    
    # 初始化累计时间变量
    total_keygen=0
    total_signing=0
    total_verification=0
    
    # 运行100次测试
    for i in $(seq 1 $iterations); do
        if [ $((i % 10)) -eq 0 ]; then
            echo "Progress: $i/$iterations"
        fi
        
        # 运行测试并捕获输出
        output=$(./$test_program 2>&1)
        
        # 提取时间数据（基于实际输出格式）
        keygen_time=$(echo "$output" | grep "KeyGen time:" | grep -oE '[0-9]+\.?[0-9]*')
        signing_time=$(echo "$output" | grep "Signing time:" | grep -oE '[0-9]+\.?[0-9]*')
        verification_time=$(echo "$output" | grep "Verification time:" | grep -oE '[0-9]+\.?[0-9]*')
        
        # 累加时间（如果提取成功）
        if [ ! -z "$keygen_time" ]; then
            total_keygen=$(echo "$total_keygen + $keygen_time" | bc -l)
        fi
        if [ ! -z "$signing_time" ]; then
            total_signing=$(echo "$total_signing + $signing_time" | bc -l)
        fi
        if [ ! -z "$verification_time" ]; then
            total_verification=$(echo "$total_verification + $verification_time" | bc -l)
        fi
    done
    
    # 计算并显示平均值
    echo ""
    echo "Results for $test_name:"
    echo "----------------------"
    
    if [ $(echo "$total_keygen > 0" | bc -l) -eq 1 ]; then
        avg_keygen=$(echo "scale=6; $total_keygen / $iterations" | bc -l)
        printf "Average KeyGen time:      %10.6f ms\n" $avg_keygen
    else
        echo "KeyGen time: Unable to extract data"
    fi
    
    if [ $(echo "$total_signing > 0" | bc -l) -eq 1 ]; then
        avg_signing=$(echo "scale=6; $total_signing / $iterations" | bc -l)
        printf "Average Signing time:     %10.6f ms\n" $avg_signing
    else
        echo "Signing time: Unable to extract data"
    fi
    
    if [ $(echo "$total_verification > 0" | bc -l) -eq 1 ]; then
        avg_verification=$(echo "scale=6; $total_verification / $iterations" | bc -l)
        printf "Average Verification time: %10.6f ms\n" $avg_verification
    else
        echo "Verification time: Unable to extract data"
    fi
}

# 记录开始时间
start_time=$(date +%s)

# 运行VHDSA-192f测试
run_test "$VHDSA_192F" "VHDSA-192f"

# 运行VHDSA-256f测试
run_test "$VHDSA_256F" "VHDSA-256f"

# 计算总耗时
end_time=$(date +%s)
total_time=$((end_time - start_time))

echo ""
echo "=============================================="
echo "Benchmark completed!"
echo "Total execution time: $total_time seconds" 