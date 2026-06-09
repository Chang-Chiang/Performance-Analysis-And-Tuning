// 代数变换 (Algebraic Transformation)
//
// 什么是代数变换？
//   利用代数恒等式简化表达式，减少计算量。
//   编译器在编译时自动进行代数变换，程序员也可以手动优化。
//
// 代数变换的作用：
//   1. 减少运算次数 — 简化表达式，减少加法、乘法等操作
//   2. 降低延迟 — 更少的运算意味着更短的执行时间
//   3. 减少寄存器压力 — 简化后的表达式需要更少的临时变量
//
// 示例分析：
//   优化前：
//     a = (a + a) + (6 * a) / 2;
//     // 计算过程：a + a = 2a, 6 * a = 6a, 6a / 2 = 3a, 2a + 3a = 5a
//
//   优化后（代数变换）：
//     a = 5 * a;
//     // 直接计算 5a，减少运算次数
//
// 编译命令：
//   g++ -O2 algebraic_transformation.cpp -o algebraic_transformation

#include <stdio.h>

// 示例 1：优化前（复杂表达式）
void example_before() {
    int a = 2, b = 3;

    // 优化前：需要多次运算
    // a = (a + a) + (6 * a) / 2;
    // 计算：a + a = 2a, 6 * a = 6a, 6a / 2 = 3a, 2a + 3a = 5a
    a = (a + a) + (6 * a) / 2;
    b = (b + b) + (6 * b) / 2;

    printf("Before: a = %d, b = %d\n", a, b);
}

// 示例 2：优化后（代数变换）
void example_after() {
    int a = 2, b = 3;

    // 优化后：代数变换简化为 5a
    a = 5 * a;
    b = 5 * b;

    printf("After:  a = %d, b = %d\n", a, b);
}

int main() {
    example_before();
    example_after();
    return 0;
}
