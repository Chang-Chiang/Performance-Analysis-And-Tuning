// 公共子表达式消除 (Common Subexpression Elimination, CSE)
//
// 什么是公共子表达式消除？
//   如果一个表达式在之前已经计算过，且其操作数没有改变，
//   则可以直接使用之前的结果，避免重复计算。
//
// 公共子表达式消除的作用：
//   1. 减少重复计算 — 相同的表达式只计算一次
//   2. 降低指令数 — 减少加法、乘法等运算指令
//   3. 提升性能 — 更少的计算意味着更短的执行时间
//
// 示例分析：
//   优化前（重复计算 a + b）：
//     if ((a + b) > 3 && (a + b) < 10) {
//         a = a + b;  // 第三次计算 a + b
//     }
//
//   优化后（公共子表达式消除）：
//     int temp = a + b;  // 只计算一次
//     if (temp > 3 && temp < 10) {
//         a = temp;
//     }
//
// 编译命令：
//   g++ -O2 cse.cpp -o cse

#include <stdio.h>

// 示例 1：优化前（重复计算）
void example_before() {
    int a = 1, b = 5;

    // 重复计算 a + b 三次
    if ((a + b) > 3 && (a + b) < 10) {
        a = a + b;
    }

    printf("Before: a = %d\n", a);
}

// 示例 2：优化后（公共子表达式消除）
void example_after() {
    int a = 1, b = 5;

    // 只计算一次 a + b
    int temp = a + b;
    if (temp > 3 && temp < 10) {
        a = temp;
    }

    printf("After:  a = %d\n", a);
}

int main() {
    example_before();
    example_after();
    return 0;
}
