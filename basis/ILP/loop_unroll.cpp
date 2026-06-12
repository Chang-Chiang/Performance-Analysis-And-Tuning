// 循环展开 (Loop Unrolling)
//
// 什么是循环展开？
//   将循环体复制多次，减少循环迭代次数，从而减少分支判断和循环控制开销。
//
// 循环展开的作用：
//   1. 减少分支开销 — 每次迭代的条件判断和跳转指令减少
//   2. 提升指令级并行 — 多个计算操作可以同时执行
//   3. 便于向量化 — 展开后可以使用 SIMD 指令
//
// 示例分析：
//   优化前（循环）：
//     for (i = 0; i < N; i++) { B[i] = A[i]; }
//
//   优化后（完全展开 + SSE 向量化）：
//     __m128 v1 = _mm_load_ps(&A[0]);
//     _mm_store_ps(&B[0], v1);
//     v1 = _mm_load_ps(&A[4]);
//     _mm_store_ps(&B[4], v1);
//
// 编译命令：
//   g++ -O2 -msse -Wall -o loop_unroll loop_unroll.cpp

#include <immintrin.h>
#include <stdio.h>

#define N 8

// 示例 1：标量循环
void example_scalar() {
    float A[N], B[N];

    for (int i = 0; i < N; i++) {
        A[i] = i * 2;
    }

    // 标量复制
    for (int i = 0; i < N; i++) {
        B[i] = A[i];
    }

    printf("Scalar: ");
    for (int i = 0; i < N; i++) printf("%.1f ", B[i]);
    printf("\n");
}

// 示例 2：完全展开 + SSE 向量化
void example_vector() {
    float A[N], B[N];

    for (int i = 0; i < N; i++) {
        A[i] = i * 2;
    }

    // 完全展开 + SSE 向量化（每次处理 4 个 float）
    __m128 v1 = _mm_loadu_ps(&A[0]);
    _mm_storeu_ps(&B[0], v1);
    v1 = _mm_loadu_ps(&A[4]);
    _mm_storeu_ps(&B[4], v1);

    printf("Vector: ");
    for (int i = 0; i < N; i++) printf("%.1f ", B[i]);
    printf("\n");
}

int main() {
    example_scalar();
    example_vector();
    return 0;
}
