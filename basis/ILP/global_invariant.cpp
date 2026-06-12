// 全局不变量合并 (Global Invariant Merging)
//
// 什么是全局不变量合并？
//   将多个循环中使用的相同不变量合并，避免重复计算或加载。
//   不变量是指在循环中值不改变的表达式或变量。
//
// 全局不变量合并的作用：
//   1. 减少重复计算 — 相同的不变量只计算一次
//   2. 减少寄存器压力 — 合并后需要更少的寄存器
//   3. 提升性能 — 更少的计算意味着更短的执行时间
//
// 示例分析：
//   优化前（不变量重复加载）：
//     for (i = 0; i < N; i += 4) {
//         V1 = _mm_set_ps(C, C, C, C);  // 每次迭代都加载
//         V2 = _mm_load_ps(&b[i]);
//         V3 = _mm_mul_ps(V1, V2);
//         _mm_store_ps(&a[i], V3);
//     }
//     for (i = 0; i < N; i += 4) {
//         V1 = _mm_set_ps(C, C, C, C);  // 重复加载
//         V2 = _mm_load_ps(&x[i]);
//         V3 = _mm_add_ps(V1, V2);
//         _mm_store_ps(&d[i], V3);
//     }
//
//   优化后（不变量合并）：
//     V1 = _mm_set_ps(C, C, C, C);  // 只加载一次
//     for (i = 0; i < N; i += 4) {
//         V2 = _mm_load_ps(&b[i]);
//         V3 = _mm_mul_ps(V1, V2);
//         _mm_store_ps(&a[i], V3);
//     }
//     for (i = 0; i < N; i += 4) {
//         V2 = _mm_load_ps(&x[i]);
//         V3 = _mm_add_ps(V1, V2);   // 重用 V1
//         _mm_store_ps(&d[i], V3);
//     }
//
// 编译命令：
//   g++ -O2 -msse -Wall -o global_invariant global_invariant.cpp

#include <immintrin.h>
#include <stdio.h>

#define N 100

// 示例 1：标量不变量
void example_scalar() {
    float a[N], b[N];
    float C = 3;

    for (int i = 0; i < N; i++) b[i] = i;

    for (int i = 0; i < N; i++) {
        a[i] = C * b[i];  // C 是不变量
    }

    printf("Scalar: a[0]=%.2f, a[1]=%.2f\n", a[0], a[1]);
}

// 示例 2：SSE 向量化不变量
void example_vector() {
    float a[N], b[N];
    float C = 3;

    for (int i = 0; i < N; i++) b[i] = i;

    // 不变量 C 加载到向量寄存器
    __m128 V1 = _mm_set1_ps(C);
    for (int i = 0; i < N; i += 4) {
        __m128 V2 = _mm_loadu_ps(&b[i]);
        __m128 V3 = _mm_mul_ps(V1, V2);
        _mm_storeu_ps(&a[i], V3);
    }

    printf("Vector: a[0]=%.2f, a[1]=%.2f\n", a[0], a[1]);
}

// 示例 3：不变量重复加载（未合并）
void example_before() {
    float a[N], b[N], d[N], x[N];
    float C = 3;

    for (int i = 0; i < N; i++) { b[i] = i; x[i] = i + 1; }

    // 第一个循环：加载不变量
    __m128 V1 = _mm_set1_ps(C);
    for (int i = 0; i < N; i += 4) {
        __m128 V2 = _mm_loadu_ps(&b[i]);
        __m128 V3 = _mm_mul_ps(V1, V2);
        _mm_storeu_ps(&a[i], V3);
    }

    // 第二个循环：重复加载不变量
    V1 = _mm_set1_ps(C);  // 重复加载
    for (int i = 0; i < N; i += 4) {
        __m128 V2 = _mm_loadu_ps(&x[i]);
        __m128 V3 = _mm_add_ps(V1, V2);
        _mm_storeu_ps(&d[i], V3);
    }

    printf("Before: a[0]=%.2f, d[0]=%.2f\n", a[0], d[0]);
}

// 示例 4：不变量合并
void example_after() {
    float a[N], b[N], d[N], x[N];
    float C = 3;

    for (int i = 0; i < N; i++) { b[i] = i; x[i] = i + 1; }

    // 不变量只加载一次
    __m128 V1 = _mm_set1_ps(C);

    for (int i = 0; i < N; i += 4) {
        __m128 V2 = _mm_loadu_ps(&b[i]);
        __m128 V3 = _mm_mul_ps(V1, V2);
        _mm_storeu_ps(&a[i], V3);
    }

    for (int i = 0; i < N; i += 4) {
        __m128 V2 = _mm_loadu_ps(&x[i]);
        __m128 V3 = _mm_add_ps(V1, V2);  // 重用 V1
        _mm_storeu_ps(&d[i], V3);
    }

    printf("After:  a[0]=%.2f, d[0]=%.2f\n", a[0], d[0]);
}

int main() {
    example_scalar();
    example_vector();
    example_before();
    example_after();
    return 0;
}
