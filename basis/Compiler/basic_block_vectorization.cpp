// 基本块向量化 (Basic Block Vectorization)
// 也称为超字级并行 (Superword Level Parallelism, SLP)
//
// 什么是基本块向量化？
//   在单个基本块内，将多条相同类型的标量操作合并为一条向量操作。
//   与循环向量化不同，基本块向量化不需要循环结构。
//
// 基本块向量化的作用:
//   1. 挖掘基本块内的并行性 — 同一基本块内的独立操作可以并行执行
//   2. 不依赖循环结构 — 适用于非循环场景
//   3. 与循环向量化互补 — 循环向量化处理循环，基本块向量化处理基本块
//
// 优化前:
//   a[i]     = b[i]     + c[i];      // 4 条独立的标量加法
//   a[i + 1] = b[i + 1] + c[i + 1];
//   a[i + 2] = b[i + 2] + c[i + 2];
//   a[i + 3] = b[i + 3] + c[i + 3];
//
// 优化后:
//   vector_a = vector_b + vector_c;  // 1 条向量加法，同时处理 4 个元素
//
// 生成 LLVM IR
//   clang -emit-llvm -S -Xclang -disable-O0-optnone basic_block_vectorization.cpp -o basic_block_vectorization.ll
//
// 执行基本块向量化优化
//   opt -passes='mem2reg,slp-vectorizer' basic_block_vectorization.ll -S -o basic_block_vectorization_opt.ll
//
// 选项:
//   -emit-llvm             生成 LLVM IR 而非本地机器码
//   -S                     输出文本格式
//   -Xclang -disable-O0-optnone  禁止添加 optnone 属性
//   -passes='mem2reg,slp-vectorizer'  先提升内存访问为寄存器，再执行 SLP 向量化

#include <stdio.h>

#define N 10240

int main() {
    int a[N], b[N], c[N];

    for (int i = 0; i < N; i++) {
        b[i] = i;
        c[i] = i + 1;
    }

    // 4 条独立的标量加法，可被 SLP 向量化为 1 条向量加法
    for (int i = 0; i < N; i += 4) {
        a[i]     = b[i]     + c[i];
        a[i + 1] = b[i + 1] + c[i + 1];
        a[i + 2] = b[i + 2] + c[i + 2];
        a[i + 3] = b[i + 3] + c[i + 3];
    }

    return a[100];
}
