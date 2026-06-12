// 循环向量化 (Loop Vectorization)
//
// 什么是循环向量化？
//   将标量循环操作转换为向量操作，利用 SIMD 指令同时处理多个数据元素。
//
// 循环向量化的作用：
//   1. 提升吞吐量 — 一条指令处理多个数据，理论性能提升 4~8 倍
//   2. 利用 SIMD 硬件 — 充分使用 SSE/AVX/NEON 等向量指令集
//   3. 减少循环次数 — 迭代次数减少 4~8 倍，分支开销降低
//
// 示例 1：标量数组复制
//   for (i = 0; i < N; i++) { a[i] = b[i]; }
//
// 示例 2：SSE 向量化数组复制
//   for (i = 0; i < N/4; i++) {
//       __m128 v = _mm_load_ps(b + i*4);
//       _mm_store_ps(a + i*4, v);
//   }
//
// 示例 3：SSE 向量化矩阵乘法（按行向量化）
//   for (i = 0; i < N; i++) {
//       for (j = 0; j < N; j += 4) {
//           __m128 sum = _mm_setzero_ps();
//           for (k = 0; k < N; k++) {
//               __m128 va = _mm_set1_ps(a[i][k]);
//               __m128 vb = _mm_load_ps(&b[k][j]);
//               sum = _mm_add_ps(sum, _mm_mul_ps(va, vb));
//           }
//           _mm_store_ps(&c[i][j], sum);
//       }
//   }
//
// 编译命令：
//   g++ -O2 -msse -Wall -o loop_vectorization loop_vectorization.cpp

#include <immintrin.h>
#include <stdio.h>

#define N 100

// 示例 1：标量数组复制
void example_scalar() {
    float a[N], b[N];

    for (int i = 0; i < N; i++) {
        b[i] = i * 3;
    }

    // 标量复制
    for (int i = 0; i < N; i++) {
        a[i] = b[i];
    }

    printf("Scalar: a[0]=%f, b[0]=%f\n", a[0], b[0]);
}

// 示例 2：SSE 向量化数组复制
void example_vector() {
    float a[N], b[N];

    for (int i = 0; i < N; i++) {
        b[i] = i * 3;
    }

    // SSE 向量化复制（每次处理 4 个 float）
    for (int i = 0; i < N / 4; i++) {
        __m128 v = _mm_loadu_ps(b + i * 4);
        _mm_storeu_ps(a + i * 4, v);
    }

    printf("Vector: a[0]=%f, b[0]=%f\n", a[0], b[0]);
}

// 示例 3：SSE 向量化矩阵乘法
void example_matrix() {
    const int M = 4;
    float a[M][M], b[M][M], c[M][M];

    // 初始化矩阵
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < M; j++) {
            a[i][j] = i + j;
            b[i][j] = i - j;
        }
    }

    // SSE 向量化矩阵乘法
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < M; j += 4) {
            __m128 sum = _mm_setzero_ps();
            for (int k = 0; k < M; k++) {
                __m128 va = _mm_set1_ps(a[i][k]);
                __m128 vb = _mm_loadu_ps(&b[k][j]);
                sum = _mm_add_ps(sum, _mm_mul_ps(va, vb));
            }
            _mm_storeu_ps(&c[i][j], sum);
        }
    }

    printf("Matrix C[0][0] = %f\n", c[0][0]);
}

int main() {
    example_scalar();
    example_vector();
    example_matrix();
    return 0;
}
