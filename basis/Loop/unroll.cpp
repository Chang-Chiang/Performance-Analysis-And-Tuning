// 循环展开 (Loop Unrolling)
//
// 什么是循环展开？
//   将循环体复制多次，减少循环迭代次数，从而减少分支判断和循环控制开销。
//
// 循环展开的作用：
//   1. 减少分支开销 — 每次迭代的条件判断和跳转指令减少
//   2. 提升指令级并行 — 多个计算操作可以同时执行（流水线、超标量）
//   3. 便于向量化 — 展开后循环体更大，编译器更容易识别向量化机会
//   4. 减少循环变量更新 — i++、比较、跳转等操作次数减少
//
// 注意事项：
//   展开次数太多，运算过程的中间变量增加，可能导致寄存器溢出，反而降低性能。
//
// 编译命令：
//   g++ -O2 -mavx2 -Wall -o unroll unroll.cpp

#include <immintrin.h>
#include <stdio.h>

#define N 256

// 示例 1：标量循环展开
void example_scalar() {
    double A[N][N], B[N][N], C[N][N];

    // 初始化矩阵
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            A[i][j] = 1.0;
            B[i][j] = 2.0;
            C[i][j] = 3.0;
        }
    }

    // 优化前：
    // for (int i = 0; i < N; i++) {
    //     for (int j = 0; j < N; j++) {
    //         A[i][j] = A[i][j] + B[i][j] * C[i][j];
    //     }
    // }

    // 优化后：循环展开 4 次
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j += 4) {
            A[i][j]     = A[i][j] + B[i][j] * C[i][j];
            A[i][j + 1] = A[i][j + 1] + B[i][j + 1] * C[i][j + 1];
            A[i][j + 2] = A[i][j + 2] + B[i][j + 2] * C[i][j + 2];
            A[i][j + 3] = A[i][j + 3] + B[i][j + 3] * C[i][j + 3];
        }

        // 处理剩余元素（尾部循环）
        for (int j = (N / 4) * 4; j < N; j++) {
            A[i][j] = A[i][j] + B[i][j] * C[i][j];
        }
    }

    printf("Scalar result: A[0][0] = %f\n", A[0][0]);
}

// 示例 2：AVX2 向量化循环展开
void example_vector() {
    double A[N][N], B[N][N], C[N][N];

    // 初始化向量寄存器
    // _mm256_set1_pd(val) — 将 256 位寄存器的 4 个 double 都设置为 val
    //   ymm0 = [1.0, 1.0, 1.0, 1.0]
    //   ymm1 = [2.0, 2.0, 2.0, 2.0]
    //   ymm2 = [3.0, 3.0, 3.0, 3.0]
    __m256d ymm0 = _mm256_set1_pd(1.0);
    __m256d ymm1 = _mm256_set1_pd(2.0);
    __m256d ymm2 = _mm256_set1_pd(3.0);

    // 使用 AVX2 初始化矩阵
    // _mm256_storeu_pd(addr, val) — 将 256 位寄存器的 4 个 double 存储到内存（未对齐）
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j += 4) {
            _mm256_storeu_pd(&A[i][j], ymm0);  // A[i][j..j+3] = [1.0, 1.0, 1.0, 1.0]
            _mm256_storeu_pd(&B[i][j], ymm1);  // B[i][j..j+3] = [2.0, 2.0, 2.0, 2.0]
            _mm256_storeu_pd(&C[i][j], ymm2);  // C[i][j..j+3] = [3.0, 3.0, 3.0, 3.0]
        }
    }

    // AVX2 向量化计算：A = A + B * C
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j += 4) {
            // _mm256_loadu_pd(addr) — 从内存加载 4 个 double 到 256 位寄存器
            __m256d a = _mm256_loadu_pd(&A[i][j]);  // a = A[i][j..j+3]
            __m256d b = _mm256_loadu_pd(&B[i][j]);  // b = B[i][j..j+3]
            __m256d c = _mm256_loadu_pd(&C[i][j]);  // c = C[i][j..j+3]

            // _mm256_mul_pd(a, b) — 4 个 double 并行乘法
            b = _mm256_mul_pd(b, c);  // b = B[i][j..j+3] * C[i][j..j+3]

            // _mm256_add_pd(a, b) — 4 个 double 并行加法
            a = _mm256_add_pd(a, b);  // a = A[i][j..j+3] + b

            _mm256_storeu_pd(&A[i][j], a);  // A[i][j..j+3] = a
        }
    }

    printf("Vector result: A[0][0] = %f\n", A[0][0]);
}

int main() {
    example_scalar();
    example_vector();
    return 0;
}
