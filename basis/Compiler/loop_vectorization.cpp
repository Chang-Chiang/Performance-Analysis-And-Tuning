// 循环向量化 (Loop Vectorization)
//
// 什么是循环向量化？
//   将标量循环操作转换为向量操作，利用 SIMD 指令同时处理多个数据元素。
//   例如：一次循环迭代处理 4 个 int（128 位 SSE）或 8 个 int（256 位 AVX）
//
// 循环向量化的作用:
//   1. 提升吞吐量 — 一条指令处理多个数据，理论性能提升 4~8 倍
//   2. 利用 SIMD 硬件 — 充分使用 SSE/AVX/NEON 等向量指令集
//   3. 减少循环次数 — 迭代次数减少 4~8 倍，分支开销降低
//
// 优化前:
//   for (j = 0; j < N; j++) {
//       sum += a[j];    // 标量：每次处理 1 个元素
//   }
//
// 优化后（向量化，假设 4 路）:
//   int sum_vec[4] = {0, 0, 0, 0};
//   for (j = 0; j < N; j += 4) {
//       sum_vec += a[j:j+3];  // 向量：每次处理 4 个元素
//   }
//   sum = sum_vec[0] + sum_vec[1] + sum_vec[2] + sum_vec[3];
//
// 生成 LLVM IR
//   clang -emit-llvm -S -Xclang -disable-O0-optnone loop_vectorization.cpp -o loop_vectorization.ll
//
// 执行循环向量化优化
//   opt -passes='mem2reg,loop-vectorize' loop_vectorization.ll -S -o loop_vectorization_opt.ll
//
// 选项:
//   -emit-llvm             生成 LLVM IR 而非本地机器码
//   -S                     输出文本格式
//   -Xclang -disable-O0-optnone  禁止添加 optnone 属性
//   -passes='mem2reg,loop-vectorize'  先提升内存访问为寄存器，再执行向量化

#include <stdio.h>

#define N 1280

int main() {
    int sum = 0;
    int a[N];

    for (int i = 0; i < N; i++) {
        a[i] = i;
    }

    for (int j = 0; j < N; j++) {
        sum = sum + a[j];
    }

    printf("sum = %d", sum);
    return 0;
}
