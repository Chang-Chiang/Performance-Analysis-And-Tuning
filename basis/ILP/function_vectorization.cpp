// 函数向量化 (Function Vectorization)
//
// 什么是函数向量化？
//   将标量函数改写为向量函数，使其可以处理 SIMD 向量数据。
//   向量化后的函数一次处理多个数据元素，提升计算效率。
//
// 函数向量化的作用：
//   1. 提升函数调用效率 — 一次调用处理多个数据
//   2. 减少函数调用开销 — 调用次数减少 N 倍
//   3. 利用 SIMD 并行 — 充分使用向量指令集
//
// 示例分析：
//   优化前（标量函数）：
//     float fun1(float x, float y) { return x * y; }
//     for (i = 0; i < N; i++) { a[i] = fun1(b[i], c[i]); }
//
//   优化后（向量化函数）：
//     __m128 vecfun1(__m128 x, __m128 y) { return _mm_mul_ps(x, y); }
//     for (i = 0; i < N; i += 4) {
//         __m128 vb = _mm_loadu_ps(&b[i]);
//         __m128 vc = _mm_loadu_ps(&c[i]);
//         __m128 va = vecfun1(vb, vc);
//         _mm_storeu_ps(&a[i], va);
//     }
//
// 编译命令：
//   g++ -O2 -msse -Wall -o function_vectorization function_vectorization.cpp

#include <immintrin.h>
#include <stdio.h>

#define N 8

// 示例 1：标量函数
float fun1_scalar(float x, float y) {
    return x * y;
}

void example_scalar() {
    float c[N], b[N], a[N];

    for (int i = 0; i < N; i++) {
        b[i] = i * 2;
        c[i] = i / 2;
    }

    // 标量函数调用
    for (int i = 0; i < N; i++) {
        a[i] = fun1_scalar(b[i], c[i]);
    }

    printf("Scalar:  ");
    for (int i = 0; i < N; i++) printf("%.1f ", a[i]);
    printf("\n");
}

// 示例 2：向量化函数
__m128 fun1_vector(__m128 x, __m128 y) {
    return _mm_mul_ps(x, y);
}

void example_vector() {
    float c[N], b[N], a[N];

    for (int i = 0; i < N; i++) {
        b[i] = i * 2;
        c[i] = i / 2;
    }

    // 向量化函数调用（每次处理 4 个 float）
    for (int i = 0; i < N; i += 4) {
        __m128 vb = _mm_loadu_ps(&b[i]);
        __m128 vc = _mm_loadu_ps(&c[i]);
        __m128 va = fun1_vector(vb, vc);
        _mm_storeu_ps(&a[i], va);
    }

    printf("Vector:  ");
    for (int i = 0; i < N; i++) printf("%.1f ", a[i]);
    printf("\n");
}

int main() {
    example_scalar();
    example_vector();
    return 0;
}
