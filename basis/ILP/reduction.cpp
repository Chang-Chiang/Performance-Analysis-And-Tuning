// 归约操作向量化 (Vectorized Reduction)
//
// 什么是归约操作？
//   将数组中的多个元素通过某种运算（如求和、求最大值）合并为一个标量结果。
//
// 归约向量化的作用：
//   1. 提升归约效率 — 利用 SIMD 指令并行处理多个元素
//   2. 减少循环次数 — 每次迭代处理多个元素
//   3. 充分利用向量寄存器 — 同时累加多个部分和
//
// 示例分析：
//   优化前（标量归约）：
//     for (j = 0; j < N; j++) { sum += a[j]; }
//
//   优化后（SSE 向量化归约）：
//     for (i = 0; i < N/4; i++) {
//         __m128 v = _mm_load_ps(a + 4*i);
//         __m128 h = _mm_hadd_ps(v, zero);  // 水平加法
//         __m128 h2 = _mm_hadd_ps(h, zero); // 再次水平加法
//         _mm_store_ps(s, h2);
//         sum += s[0];
//     }
//
// 编译命令：
//   g++ -O2 -msse -Wall -o reduction reduction.cpp

#include <immintrin.h>
#include <stdio.h>

#define N 128

// 示例 1：标量归约
void example_scalar() {
    float a[N];
    float sum = 0;

    for (int i = 0; i < N; i++) {
        a[i] = i + 1;
    }

    for (int j = 0; j < N; j++) {
        sum += a[j];
    }

    printf("Scalar: sum = %f\n", sum);
}

// 示例 2：SSE 向量化归约
void example_vector() {
    float a[N];
    float sum = 0;
    float s[4] = {0, 0, 0, 0};

    for (int i = 0; i < N; i++) {
        a[i] = i + 1;
    }

    // SSE 向量化归约
    for (int i = 0; i < N / 4; i++) {
        __m128 v  = _mm_loadu_ps(a + 4 * i);
        __m128 h  = _mm_hadd_ps(v, _mm_setzero_ps());   // 水平加法
        __m128 h2 = _mm_hadd_ps(h, _mm_setzero_ps());   // 再次水平加法
        _mm_storeu_ps(s, h2);
        sum += s[0];
    }

    printf("Vector: sum = %f\n", sum);
}

int main() {
    example_scalar();
    example_vector();
    return 0;
}
