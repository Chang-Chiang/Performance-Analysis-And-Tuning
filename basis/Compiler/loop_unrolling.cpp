// 循环展开 (Loop Unrolling)
//
// 什么是循环展开？
//   将循环体复制多次，减少循环迭代次数，从而减少分支判断和循环控制开销。
//
// 循环展开的作用:
//   1. 减少分支开销 — 每次迭代的条件判断和跳转指令减少
//   2. 提升指令级并行 — 多个计算操作可以同时执行（流水线、超标量）
//   3. 便于向量化 — 展开后循环体更大，编译器更容易识别向量化机会
//   4. 减少循环变量更新 — i++、比较、跳转等操作次数减少
//
// 优化前:
//   for (j = 0; j < N; j++) {
//       sum += a[j];    // 每次迭代：1 次加法 + 1 次比较 + 1 次跳转
//   }
//
// 优化后（展开 4 次）:
//   for (j = 0; j < N; j += 4) {
//       sum += a[j];      // 迭代 0
//       sum += a[j + 1];  // 迭代 1
//       sum += a[j + 2];  // 迭代 2
//       sum += a[j + 3];  // 迭代 3
//   }
//   // 循环次数从 N 减少到 N/4，分支判断减少 4 倍
//
// 生成 LLVM IR
//   clang -emit-llvm -S -Xclang -disable-O0-optnone loop_unroll.cpp -o loop_unroll.ll
//
// 执行循环展开优化
//   opt -passes='mem2reg,loop-unroll' loop_unroll.ll -S -o loop_unroll_opt.ll
//
// 选项:
//   -emit-llvm             生成 LLVM IR 而非本地机器码
//   -S                     输出文本格式
//   -Xclang -disable-O0-optnone  禁止添加 optnone 属性
//   -passes='mem2reg,loop-unroll'  先提升内存访问为寄存器，再执行循环展开

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
