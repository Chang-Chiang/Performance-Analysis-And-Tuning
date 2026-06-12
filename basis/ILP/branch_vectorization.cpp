// 分支向量化 (Branch Vectorization)
//
// 什么是分支向量化？
//   将包含分支判断的循环转换为无分支的向量操作，
//   使用条件赋值（如三元运算符）或 SIMD 比较指令消除分支。
//
// 分支向量化的作用：
//   1. 消除分支跳转 — 避免分支预测失败的惩罚
//   2. 便于向量化 — 无分支代码可以使用 SIMD 指令
//   3. 提高流水线效率 — 无分支代码更容易流水线化
//
// 示例分析：
//   优化前（有分支）：
//     for (i = 0; i < N; i++) {
//         if (a[i] > 0 && a[i] > b[i]) {
//             v[i] = w[i];
//             if (b[i] > 10) { p[i] = m[i]; }
//             else { p[i] = n[i]; if (c[i] < 100) { x[i] = y[i]; } }
//         }
//     }
//
//   优化后（If 转换）：
//     for (i = 0; i < N; i++) {
//         v[i] = (a[i] > 0 && a[i] > b[i]) ? w[i] : v[i];
//         p[i] = (a[i] > 0 && a[i] > b[i] && b[i] > 10) ? m[i] : p[i];
//         p[i] = (a[i] > 0 && a[i] > b[i] && b[i] <= 10) ? n[i] : p[i];
//         x[i] = (a[i] > 0 && a[i] > b[i] && b[i] <= 10 && c[i] < 100) ? y[i] : x[i];
//     }
//
// 编译命令：
//   g++ -O2 -msse -Wall -o branch_vectorization branch_vectorization.cpp

#include <immintrin.h>
#include <stdio.h>

#define N 12

// 示例 1：优化前（有分支）
void example_1_before() {
    float a[N], b[N], c[N], w[N], v[N], m[N], n[N], p[N], x[N], y[N];

    for (int i = 0; i < N; i++) {
        a[i] = i + 1; b[i] = i * 2; c[i] = i * 10;
        w[i] = i + 2; v[i] = i - 1; m[i] = i + 3;
        n[i] = i - 2; p[i] = i;     x[i] = i; y[i] = i;
    }

    // 有分支判断
    for (int i = 0; i < N; i++) {
        if (a[i] > 0 && a[i] > b[i]) {
            v[i] = w[i];
            if (b[i] > 10) {
                p[i] = m[i];
            } else {
                p[i] = n[i];
                if (c[i] < 100) {
                    x[i] = y[i];
                }
            }
        }
    }

    printf("Example 1 (before): v[0]=%f, p[0]=%f, x[0]=%f\n", v[0], p[0], x[0]);
}

// 示例 2：优化后（If 转换）
void example_1_after() {
    float a[N], b[N], c[N], w[N], v[N], m[N], n[N], p[N], x[N], y[N];

    for (int i = 0; i < N; i++) {
        a[i] = i + 1; b[i] = i * 2; c[i] = i * 10;
        w[i] = i + 2; v[i] = i - 1; m[i] = i + 3;
        n[i] = i - 2; p[i] = i;     x[i] = i; y[i] = i;
    }

    // If 转换：用条件赋值替代分支
    for (int i = 0; i < N; i++) {
        v[i] = (a[i] > 0 && a[i] > b[i]) ? w[i] : v[i];
        p[i] = (a[i] > 0 && a[i] > b[i] && b[i] > 10) ? m[i] : p[i];
        p[i] = (a[i] > 0 && a[i] > b[i] && b[i] <= 10) ? n[i] : p[i];
        x[i] = (a[i] > 0 && a[i] > b[i] && b[i] <= 10 && c[i] < 100) ? y[i] : x[i];
    }

    printf("Example 1 (after):  v[0]=%f, p[0]=%f, x[0]=%f\n", v[0], p[0], x[0]);
}

// 示例 3：标量分支
void example_2_before() {
    const int M = 16;
    float a[M], b[M], c[M];

    for (int i = 0; i < M; i++) {
        a[i] = 1; b[i] = 3 + i; c[i] = 1;
    }

    // 有分支判断
    for (int i = 0; i < M; i += 2) {
        a[i]     = 2 * b[i];
        a[i + 1] = 2 * b[i + 1];
        if (i < M / 2) {
            a[i] += b[i]; a[i + 1] += b[i + 1];
        } else {
            a[i] -= b[i]; a[i + 1] -= b[i + 1];
        }
        c[i]     = a[i] + 2;
        c[i + 1] = a[i + 1] + 2;
    }

    printf("Example 2 (before): a[0]=%f, c[0]=%f\n", a[0], c[0]);
}

// 示例 4：SSE 向量化（带分支）
void example_2_after() {
    const int M = 16;
    float a[M], b[M], c[M];

    for (int i = 0; i < M; i++) {
        a[i] = 1; b[i] = 3 + i; c[i] = 1;
    }

    // SSE 向量化（仍有分支）
    for (int i = 0; i < M; i += 4) {
        __m128 vb = _mm_loadu_ps(&b[i]);
        __m128 v2 = _mm_set1_ps(2.0f);
        __m128 va = _mm_mul_ps(vb, v2);

        if (i < M / 2) {
            va = _mm_add_ps(va, vb);
        } else {
            va = _mm_sub_ps(va, vb);
        }

        __m128 vc = _mm_add_ps(va, v2);
        _mm_storeu_ps(&a[i], va);
        _mm_storeu_ps(&c[i], vc);
    }

    printf("Example 2 (after):  a[0]=%f, c[0]=%f\n", a[0], c[0]);
}

int main() {
    example_1_before();
    example_1_after();
    example_2_before();
    example_2_after();
    return 0;
}
