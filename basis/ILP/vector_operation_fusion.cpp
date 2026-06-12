// 向量运算融合 (Vector Operation Fusion)
//
// 什么是向量运算融合？
//   将多个向量运算合并为一个运算，减少指令数和内存访问。
//   常见的融合运算是乘加融合（FMA, Fused Multiply-Add）。
//
// 向量运算融合的作用：
//   1. 减少指令数 — 一条 FMA 指令替代乘法和加法两条指令
//   2. 提高精度 — FMA 只舍入一次，比分开计算更精确
//   3. 提升性能 — 减少指令数意味着更短的执行时间
//
// 示例分析：
//   优化前（标量）：
//     for (i = 0; i < N; i++) {
//         a[i] = d[i] + b[i] * c[i];
//     }
//
//   优化后（SSE 向量化）：
//     for (i = 0; i < N; i += 4) {
//         v1 = _mm_loadu_ps(&b[i]);
//         v2 = _mm_loadu_ps(&c[i]);
//         v3 = _mm_mul_ps(v1, v2);      // 乘法
//         v4 = _mm_loadu_ps(&d[i]);
//         v5 = _mm_add_ps(v4, v3);      // 加法
//         _mm_storeu_ps(&a[i], v5);
//     }
//
//   优化后（FMA 融合）：
//     for (i = 0; i < N; i += 4) {
//         v1 = _mm_load_ps(&b[i]);
//         v2 = _mm_load_ps(&c[i]);
//         v3 = _mm_load_ps(&d[i]);
//         v3 = _mm_fmadd_ps(v1, v2, v3); // 乘加融合：v3 = v1 * v2 + v3
//         _mm_store_ps(&a[i], v3);
//     }
//
// 编译命令：
//   g++ -O2 -mfma -Wall -o vector_operation_fusion vector_operation_fusion.cpp

#include <immintrin.h>
#include <stdio.h>

#define N 100

// 示例 1：标量运算
void example_scalar() {
    float a[N], b[N], c[N], d[N];

    for (int i = 0; i < N; i++) {
        b[i] = 2; c[i] = i; d[i] = -i;
    }

    for (int i = 0; i < N; i++) {
        a[i] = d[i] + b[i] * c[i];
    }

    printf("Scalar: a[0]=%.2f, a[1]=%.2f\n", a[0], a[1]);
}

// 示例 2：SSE 向量化（乘法 + 加法）
void example_sse() {
    float a[N], b[N], c[N], d[N];

    for (int i = 0; i < N; i++) {
        b[i] = 2; c[i] = i; d[i] = -i;
    }

    // 乘法和加法分开执行
    for (int i = 0; i < N; i += 4) {
        __m128 v1 = _mm_loadu_ps(&b[i]);
        __m128 v2 = _mm_loadu_ps(&c[i]);
        __m128 v3 = _mm_mul_ps(v1, v2);      // 乘法
        __m128 v4 = _mm_loadu_ps(&d[i]);
        __m128 v5 = _mm_add_ps(v4, v3);      // 加法
        _mm_storeu_ps(&a[i], v5);
    }

    printf("SSE:    a[0]=%.2f, a[1]=%.2f\n", a[0], a[1]);
}

// 示例 3：FMA 融合乘加
void example_fma() {
    float a[N], b[N], c[N], d[N];

    for (int i = 0; i < N; i++) {
        b[i] = 2; c[i] = i; d[i] = -i;
    }

    // 乘加融合：一条指令完成 d + b * c
    for (int i = 0; i < N; i += 4) {
        __m128 v1 = _mm_loadu_ps(&b[i]);
        __m128 v2 = _mm_loadu_ps(&c[i]);
        __m128 v3 = _mm_loadu_ps(&d[i]);
        v3 = _mm_fmadd_ps(v1, v2, v3);  // FMA: v3 = v1 * v2 + v3
        _mm_storeu_ps(&a[i], v3);
    }

    printf("FMA:    a[0]=%.2f, a[1]=%.2f\n", a[0], a[1]);
}

int main() {
    example_scalar();
    example_sse();
    example_fma();
    return 0;
}
