// 循环分段 (Loop Tiling / Loop Blocking)
//
// 什么是循环分段？
//   将单层循环变换为嵌套循环，将大循环拆分为多个小段（tile）。
//   每个小段的数据可以完全放入缓存，提升缓存命中率。
//
// 循环分段的作用：
//   1. 提升缓存利用率 — 每个小段的数据在缓存中处理，减少缓存未命中
//   2. 减少内存访问 — 数据在缓存中重用，减少对主存的访问
//   3. 便于向量化 — 小段内的数据可以更好地利用 SIMD 指令
//
// 优化前：
//   for (i = 0; i < N; i++) {
//       A[i] = B[i] + C[i];
//   }
//
// 优化后（分段大小 K = 32）：
//   for (i = 0; i < N; i += K) {
//       for (j = i; j < i + K; j += 4) {
//           // 使用 SSE 向量化处理 4 个 float
//       }
//   }
//
// 编译命令：
//   g++ -O2 -msse -Wall -o segment segment.cpp

#include <immintrin.h>
#include <stdio.h>

#define N 256

int main() {
    float A[N], B[N], C[N];

    // 初始化数组
    for (int i = 0; i < N; i++) {
        A[i] = 1.0;
        B[i] = 2.0;
        C[i] = 3.0;
    }

    // 优化前：
    // for (int i = 0; i < N; i++) {
    //     A[i] = B[i] + C[i];
    // }

    // 优化后：循环分段 + SSE 向量化
    int K = 32;  // 分段大小（应为缓存行大小的整数倍）
    for (int i = 0; i < N; i += K) {
        for (int j = i; j < i + K; j += 4) {
            // _mm_load_ps — 加载 4 个 float（对齐）
            __m128 b = _mm_load_ps(&B[j]);
            __m128 c = _mm_load_ps(&C[j]);

            // _mm_add_ps — 4 个 float 并行加法
            __m128 a = _mm_add_ps(b, c);

            // _mm_storeu_ps — 存储 4 个 float（未对齐）
            _mm_storeu_ps(&A[j], a);
        }
    }

    printf("Result: ");
    for (int i = 0; i < N; i++) {
        printf("%.1f ", A[i]);
    }
    printf("\n");

    return 0;
}
