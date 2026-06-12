// SSE 矩阵乘法性能比较 (SSE Matrix Multiplication Performance)
//
// 本例比较 float 和 double 两种数据类型使用 SSE 指令进行矩阵乘法的性能差异。
//
// 优化策略：
//   1. 矩阵转置 — 将 B 矩阵转置，使内存访问连续，提升缓存命中率
//   2. SIMD 并行 — 使用 SSE 指令同时处理多个数据
//   3. 数据类型 — float 使用 4 路并行，double 使用 2 路并行
//
// SSE 指令说明（float）：
//   _mm_loadu_ps     — 加载 4 个 float（128 位）
//   _mm_mul_ps       — 4 个 float 并行乘法
//   _mm_add_ps       — 4 个 float 并行加法
//   _mm_hadd_ps      — 水平加法（将 4 个元素两两相加）
//   _mm_store_ss     — 存储 1 个 float
//
// SSE 指令说明（double）：
//   _mm_loadu_pd     — 加载 2 个 double（128 位）
//   _mm_mul_pd       — 2 个 double 并行乘法
//   _mm_add_pd       — 2 个 double 并行加法
//   _mm_hadd_pd      — 水平加法（将 2 个元素相加）
//   _mm_store_sd     — 存储 1 个 double
//
// 编译命令：
//   g++ -O2 -msse2 -Wall -o mul mul.cpp

#include <immintrin.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// 交换两个 float 指针
void swap(float* a, float* b) {
    float temp = *a;
    *a = *b;
    *b = temp;
}

// 交换两个 double 指针
void swap_1(double* a, double* b) {
    double temp = *a;
    *a = *b;
    *b = temp;
}

// SSE 优化的 float 矩阵乘法
void sse_mul(int n, float** a, float** b, float** c) {
    __m128 t1, t2, sum;

    // 转置矩阵 B，使内存访问连续
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < i; ++j) {
            swap(&b[i][j], &b[j][i]);
        }
    }

    // 矩阵乘法 C = A * B^T
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            c[i][j] = 0.0;
            sum = _mm_setzero_ps();

            // 每次处理 4 个 float
            for (int k = n - 4; k >= 0; k -= 4) {
                t1 = _mm_loadu_ps(a[i] + k);
                t2 = _mm_loadu_ps(b[j] + k);
                t1 = _mm_mul_ps(t1, t2);
                sum = _mm_add_ps(sum, t1);
            }

            // 水平求和
            sum = _mm_hadd_ps(sum, sum);
            sum = _mm_hadd_ps(sum, sum);
            _mm_store_ss(c[i] + j, sum);

            // 处理剩余元素
            for (int k = (n % 4) - 1; k >= 0; --k) {
                c[i][j] += a[i][k] * b[j][k];
            }
        }
    }

    // 恢复矩阵 B
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < i; ++j) {
            swap(&b[i][j], &b[j][i]);
        }
    }
}

// SSE 优化的 double 矩阵乘法
void sse_mul_1(int n, double** a, double** b, double** c) {
    __m128d t1, t2, sum;

    // 转置矩阵 B
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < i; ++j) {
            swap_1(&b[i][j], &b[j][i]);
        }
    }

    // 矩阵乘法 C = A * B^T
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            c[i][j] = 0.0;
            sum = _mm_setzero_pd();

            // 每次处理 2 个 double
            for (int k = n - 2; k >= 0; k -= 2) {
                t1 = _mm_loadu_pd(a[i] + k);
                t2 = _mm_loadu_pd(b[j] + k);
                t1 = _mm_mul_pd(t1, t2);
                sum = _mm_add_pd(sum, t1);
            }

            // 水平求和
            sum = _mm_hadd_pd(sum, sum);
            _mm_store_sd(c[i] + j, sum);

            // 处理剩余元素
            for (int k = (n % 2) - 1; k >= 0; --k) {
                c[i][j] += a[i][k] * b[j][k];
            }
        }
    }

    // 恢复矩阵 B
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < i; ++j) {
            swap_1(&b[i][j], &b[j][i]);
        }
    }
}

int main() {
    int n = 256;

    // 分配 float 矩阵
    float** a = (float**)malloc(n * sizeof(float*));
    float** b = (float**)malloc(n * sizeof(float*));
    float** c = (float**)malloc(n * sizeof(float*));
    for (int i = 0; i < n; i++) {
        a[i] = (float*)malloc(n * sizeof(float));
        b[i] = (float*)malloc(n * sizeof(float));
        c[i] = (float*)malloc(n * sizeof(float));
    }

    // 分配 double 矩阵
    double** a1 = (double**)malloc(n * sizeof(double*));
    double** b1 = (double**)malloc(n * sizeof(double*));
    double** c1 = (double**)malloc(n * sizeof(double*));
    for (int i = 0; i < n; i++) {
        a1[i] = (double*)malloc(n * sizeof(double));
        b1[i] = (double*)malloc(n * sizeof(double));
        c1[i] = (double*)malloc(n * sizeof(double));
    }

    // 初始化矩阵
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            a[i][j]  = rand() % 10;
            b[i][j]  = rand() % 10;
            c[i][j]  = 0;
            a1[i][j] = rand() % 10;
            b1[i][j] = rand() % 10;
            c1[i][j] = 0;
        }
    }

    printf("Matrix size: %d x %d\n", n, n);

    // 测试 float 矩阵乘法
    clock_t start = clock();
    sse_mul(n, a, b, c);
    clock_t end = clock();
    double time_float = (double)(end - start) / CLOCKS_PER_SEC;
    printf("SSE float  matrix mul: %.6f s\n", time_float);

    // 测试 double 矩阵乘法
    start = clock();
    sse_mul_1(n, a1, b1, c1);
    end = clock();
    double time_double = (double)(end - start) / CLOCKS_PER_SEC;
    printf("SSE double matrix mul: %.6f s\n", time_double);

    printf("Speed-up (double/float): %.2fx\n", time_double / time_float);

    // 释放内存
    for (int i = 0; i < n; i++) {
        free(a[i]);
        free(b[i]);
        free(c[i]);
        free(a1[i]);
        free(b1[i]);
        free(c1[i]);
    }
    free(a);
    free(b);
    free(c);
    free(a1);
    free(b1);
    free(c1);

    return 0;
}
