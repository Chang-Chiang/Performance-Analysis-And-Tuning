// 循环交换 (Loop Interchange)
//
// 什么是循环交换？
//   交换嵌套循环的顺序，使内存访问模式更加连续，增强数据局部性。
//
// 循环交换的作用：
//   1. 提升缓存命中率 — 连续访问内存比跳跃访问更高效
//   2. 便于向量化 — 连续内存访问可以使用 SIMD 指令
//   3. 减少内存延迟 — 数据在缓存中重用，减少对主存的访问
//
// 优化前（按列访问，不连续）：
//   for (i = 0; i < N; i++) {
//       for (j = 0; j < N; j++) {
//           A[i][j] = A[i-1][j];  // 按列访问，跳跃式
//       }
//   }
//
// 优化后（按行访问，连续）：
//   for (j = 0; j < N; j++) {
//       for (i = 0; i < N; i++) {
//           A[i][j] = A[i-1][j];  // 按行访问，连续
//       }
//   }
//
// 编译命令：
//   g++ -O2 -msse2 -Wall -o permutation permutation.cpp

#include <immintrin.h>
#include <stdio.h>
#include <time.h>

#define N 256

// 示例 1：矩阵乘法循环交换 + SSE 向量化
void example_matrix() {
    float A[N][N], B[N][N], C[N][N];

    // 初始化矩阵
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            A[i][j] = 1.0;
            B[i][j] = 2.0;
            C[i][j] = 3.0;
        }
    }

    // 优化前：按列访问（不连续）
    // for (int j = 0; j < N; j++) {
    //     for (int k = 0; k < N; k++) {
    //         for (int i = 0; i < N; i++) {
    //             A[i][j] = A[i][j] + B[i][k] * C[k][j];
    //         }
    //     }
    // }

    // A[i][j] = A[i][j] + B[i][k] * C[k][j];

    // 优化后：交换循环顺序 + SSE 向量化
    for (int j = 0; j < N; j++) {
        for (int i = 0; i < N; i++) {
            __m128 VA = _mm_loadu_ps(&A[i][j]);
            for (int k = 0; k < N; k++) {
                __m128 VC = _mm_loadu_ps(&C[k][j]);
                __m128 VB = _mm_set1_ps(B[i][k]);
                VB        = _mm_mul_ps(VB, VC);
                VA        = _mm_add_ps(VA, VB);
            }
            _mm_storeu_ps(&A[i][j], VA);
        }
    }

    printf("Matrix result: A[0][0] = %f\n", A[0][0]);
}

// 示例 2：按列访问 vs 按行访问性能对比
void example_access_pattern() {
    const int M = 1000;
    float     A[M][M];

    // 初始化数组
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < M; j++) {
            A[i][j] = j;
        }
    }

    // 优化前：按列访问（不连续）
    clock_t start = clock();
    for (int i = 1; i < M; i++) {
        for (int j = 1; j < M; j++) {
            A[i][j] = A[i - 1][j];
        }
    }
    clock_t end   = clock();
    double  time1 = (double)(end - start) / CLOCKS_PER_SEC;
    printf("按列访问 (优化前): %.6f s\n", time1);

    // 优化后：按行访问（连续）
    start = clock();
    for (int j = 1; j < M; j++) {
        for (int i = 1; i < M; i++) {
            A[i][j] = A[i - 1][j];
        }
    }
    end          = clock();
    double time2 = (double)(end - start) / CLOCKS_PER_SEC;
    printf("按行访问 (优化后): %.6f s\n", time2);

    printf("Speed-up: %.2fx\n", time1 / time2);
}

int main() {
    example_matrix();
    example_access_pattern();
    return 0;
}
