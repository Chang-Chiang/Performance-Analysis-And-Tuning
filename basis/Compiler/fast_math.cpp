// 快速数学优化 (Fast-math Optimization)
//
// 什么是快速数学优化？
//   放宽 IEEE 754 浮点标准的严格要求，允许编译器进行更激进的浮点优化。
//   代价是可能损失一定的精度或改变特殊值（NaN、Inf）的行为。
//
// 快速数学优化的作用:
//   1. 允许重新排列运算顺序 — 例如 (a + b) + c 可以变为 a + (b + c)
//   2. 允许强度削减 — 例如 x / y 变为 x * (1/y)
//   3. 假设无 NaN/Inf — 简化条件判断和分支
//   4. 允许融合乘加 — 使用 FMA 指令替代单独的乘法和加法
//
// 优化前（严格浮点）:
//   sum = sum + a[j];  // 按严格顺序累加，保证精度
//
// 优化后（快速数学）:
//   // 编译器可以重新排列、向量化、使用 FMA 等
//   vector_sum = vector_sum + vector_a;  // 向量化累加
//
// 生成 LLVM IR（启用快速数学）
//   clang -emit-llvm -S -ffast-math fast_math.cpp -o fast_math.ll
//
// 查看向量化报告
//   clang fast_math.cpp -fvectorize -O1 -Rpass-missed=loop-vectorize -Rpass=loop-vectorize
//
//   输出结果:
//     fast_math.cpp:45: remark: vectorized loop (vectorization width: 4)   ← 第一个循环：向量化成功
//     fast_math.cpp:49: remark: loop not vectorized                       ← 第二个循环：向量化失败
//
//   分析:
//     for (int i = 0; i < N; i++) { a[i] = i; }        // ✓ 无循环携带依赖，可向量化
//     for (int j = 0; j < N; j++) { sum += a[j]; }     // ✗ sum 依赖上一次迭代，无法向量化
//
//   解决方法: 使用 -ffast-math 允许重新排列浮点运算顺序，可消除此依赖
//     clang fast_math.cpp -fvectorize -O1 -ffast-math -Rpass=loop-vectorize
//
// 选项:
//   -emit-llvm     生成 LLVM IR 而非本地机器码
//   -S             输出文本格式
//   -ffast-math    启用快速数学优化

#include <stdio.h>

#define N 128

int main() {
    float sum = 0;
    float a[N];

    for (int i = 0; i < N; i++) {
        a[i] = i;
    }

    for (int j = 0; j < N; j++) {
        sum = sum + a[j];
    }

    printf("sum = %f", sum);
    return 0;
}
