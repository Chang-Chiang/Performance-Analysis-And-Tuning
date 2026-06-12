// 基本块向量化 (Basic Block Vectorization / SLP)
//
// 什么是基本块向量化？
//   在单个基本块内，将多条相同类型的标量操作合并为一条向量操作。
//   也称为超字级并行（Superword Level Parallelism, SLP）。
//
// 基本块向量化的作用：
//   1. 挖掘基本块内的并行性 — 同一基本块内的独立操作可以并行执行
//   2. 不依赖循环结构 — 适用于非循环场景
//   3. 与循环向量化互补 — 循环向量化处理循环，基本块向量化处理基本块
//
// 编译命令：
//   g++ -O2 -msse -Wall -o basic_block_vectorization basic_block_vectorization.cpp

#include <immintrin.h>
#include <stdio.h>

#define N 4

// 示例 1：标量操作（可向量化）
void example_1() {
    float a[N][N], v[N][N];
    float sum0 = 0, sum1 = 0, sum2 = 0;

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            a[i][j] = i + j;
            v[i][j] = i - j;
        }
    }

    // 三条独立的累加操作，可以向量化
    for (int i = 0; i < N; i++) {
        sum0 += a[0][0] * v[0][i] + a[0][1] * v[0][i] + a[0][2] * v[0][i];
        sum1 += a[1][0] * v[1][i] + a[1][1] * v[1][i] + a[1][2] * v[1][i];
        sum2 += a[2][0] * v[2][i] + a[2][1] * v[2][i] + a[2][2] * v[2][i];
    }

    printf("Example 1: sum0=%f, sum1=%f, sum2=%f\n", sum0, sum1, sum2);
}

// 示例 2：不同操作（无法向量化）
void example_2() {
    float B[N] = {0, 1, 2, 3};
    float C[N];

    // 操作不同，无法向量化
    for (int i = 0; i < N; i += 2) {
        C[i]     = B[i] * 0.5 + 2;
        C[i + 1] = B[i + 1] + 1;
    }

    printf("Example 2 (different ops): ");
    for (int i = 0; i < N; i++) printf("%.1f ", C[i]);
    printf("\n");
}

// 示例 3：相同操作（可向量化）
void example_3() {
    float B[N] = {0, 1, 2, 3};
    float C[N];

    // 操作相同，可以向量化
    for (int i = 0; i < N; i += 2) {
        C[i]     = B[i] * 0.5 + 2;
        C[i + 1] = B[i + 1] * 1 + 1;
    }

    printf("Example 3 (same ops):     ");
    for (int i = 0; i < N; i++) printf("%.1f ", C[i]);
    printf("\n");
}

// 示例 4：复数运算（可向量化）
void example_4() {
    struct Complex { float real, imag; };
    Complex A[N] = {{0, 0}, {2, 3}, {4, 6}, {6, 9}};
    Complex B[N] = {{-1, -2}, {0, -1}, {1, 0}, {2, 1}};
    Complex C[N];

    // 实部和虚部独立计算，可以向量化
    for (int i = 0; i < N; i++) {
        C[i].real = (A[i].real - B[i].real) * 0.5;
        C[i].imag = (A[i].imag + B[i].imag) * 0.5;
    }

    printf("Example 4 (complex):      ");
    for (int i = 0; i < N; i++) printf("(%.1f,%.1f) ", C[i].real, C[i].imag);
    printf("\n");
}

// 示例 5：标量循环展开
void example_5() {
    const int M = 48;
    float c[M];

    // 标量循环展开
    for (int i = 0; i < M; i += 6) {
        c[i + 0] = 0;
        c[i + 1] = 1;
        c[i + 2] = 2;
        c[i + 3] = 3;
        c[i + 4] = 4;
        c[i + 5] = 5;
    }

    printf("Example 5 (scalar unroll): ");
    for (int i = 0; i < M; i++) {
        printf("%.0f ", c[i]);
        if ((i + 1) % 6 == 0) printf("| ");
    }
    printf("\n");
}

// 示例 6：SSE 向量化（每次处理 4 个元素）
void example_6() {
    const int M = 48;
    float c[M];

    // SSE 向量化：一次处理 4 个元素
    __m128 z = _mm_set_ps(3, 2, 1, 0);

    for (int i = 0; i < M; i += 6) {
        _mm_storeu_ps(&c[i], z);
        c[i + 4] = 4;
        c[i + 5] = 5;
    }

    printf("Example 6 (SSE 4-elem):   ");
    for (int i = 0; i < M; i++) {
        printf("%.0f ", c[i]);
        if ((i + 1) % 6 == 0) printf("| ");
    }
    printf("\n");
}

// 示例 7：标量循环展开（24 个元素）
void example_7() {
    const int M = 48;
    float c[M];

    // 标量循环展开：每次处理 24 个元素
    for (int i = 0; i < M; i += 24) {
        c[i]          = 0; c[i + 6]      = 0; c[i + 12]     = 0; c[i + 18]     = 0;
        c[i + 1]      = 1; c[i + 1 + 6]  = 1; c[i + 1 + 12] = 1; c[i + 1 + 18] = 1;
        c[i + 2]      = 2; c[i + 2 + 6]  = 2; c[i + 2 + 12] = 2; c[i + 2 + 18] = 2;
        c[i + 3]      = 3; c[i + 3 + 6]  = 3; c[i + 3 + 12] = 3; c[i + 3 + 18] = 3;
        c[i + 4]      = 4; c[i + 4 + 6]  = 4; c[i + 4 + 12] = 4; c[i + 4 + 18] = 4;
        c[i + 5]      = 5; c[i + 5 + 6]  = 5; c[i + 5 + 12] = 5; c[i + 5 + 18] = 5;
    }

    printf("Example 7 (scalar 24):    ");
    for (int i = 0; i < M; i++) {
        printf("%.0f ", c[i]);
        if ((i + 1) % 6 == 0) printf("| ");
    }
    printf("\n");
}

// 示例 8：SSE 向量化（每次处理 12 个元素）
void example_8() {
    const int M = 48;
    float c[M];

    // SSE 向量化：一次处理 12 个元素（3 个 SSE 寄存器）
    __m128 z  = _mm_set_ps(3, 2, 1, 0);
    __m128 z1 = _mm_set_ps(1, 0, 5, 4);
    __m128 z2 = _mm_set_ps(5, 4, 3, 2);

    for (int i = 0; i < M; i += 24) {
        _mm_storeu_ps(&c[i], z);
        _mm_storeu_ps(&c[i + 4], z1);
        _mm_storeu_ps(&c[i + 8], z2);
        _mm_storeu_ps(&c[i + 12], z);
        _mm_storeu_ps(&c[i + 16], z1);
        _mm_storeu_ps(&c[i + 20], z2);
    }

    printf("Example 8 (SSE 12-elem):  ");
    for (int i = 0; i < M; i++) {
        printf("%.0f ", c[i]);
        if ((i + 1) % 6 == 0) printf("| ");
    }
    printf("\n");
}

// 示例 9：SSE 向量化（优化版本）
void example_9() {
    const int M = 48;
    float c[M];

    // SSE 向量化：优化版本，每次处理 12 个元素
    __m128 z  = _mm_set_ps(3, 2, 1, 0);
    __m128 z1 = _mm_set_ps(1, 0, 5, 4);
    __m128 z2 = _mm_set_ps(5, 4, 3, 2);

    for (int i = 0; i < M; i += 12) {
        _mm_storeu_ps(&c[i], z);
        _mm_storeu_ps(&c[i + 4], z1);
        _mm_storeu_ps(&c[i + 8], z2);
    }

    printf("Example 9 (SSE optimized):");
    for (int i = 0; i < M; i++) {
        printf("%.0f ", c[i]);
        if ((i + 1) % 6 == 0) printf("| ");
    }
    printf("\n");
}

int main() {
    example_1();
    example_2();
    example_3();
    example_4();
    example_5();
    example_6();
    example_7();
    example_8();
    example_9();
    return 0;
}
