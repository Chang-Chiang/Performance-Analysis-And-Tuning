/**
 * 矩阵乘法 - 缓存分块优化
 *
 * 原理：
 * 缓存分块（Cache Blocking）通过将大矩阵分解为多个小块，
 * 使得每个小块能够完全放入缓存中，从而减少缓存未命中。
 *
 * 优化前：
 * - 传统矩阵乘法，内层循环访问列，导致频繁缓存未命中
 *
 * 优化后：
 * - 将矩阵分解为 S×S 的小块
 * - 每个小块能够完全放入缓存中
 * - 减少缓存未命中，提高性能
 *
 * 编译指令：
 * gcc -O2 -o matrix_multiply_cache_blocking matrix_multiply_cache_blocking.c
 *
 * 运行：
 * ./matrix_multiply_cache_blocking
 */

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <time.h>

/* 最小值宏 */
#define min(a,b) ((a)<(b)?(a):(b))

/**
 * 缓存分块矩阵乘法
 * @param N 矩阵维度
 * @param x 结果矩阵
 * @param y 输入矩阵 1
 * @param z 输入矩阵 2
 * @param S 分块大小
 *
 * 分块策略：
 * 将矩阵分解为 S×S 的小块，每个小块能够完全放入缓存中
 * 通过调整循环顺序，使得内层循环访问的数据在缓存中
 */
void matrixmulti_blocked(int N, int** x, int** y, int** z, int S) {
    int kk, jj, i, j, k, r;

    /* 外层循环：按列分块 */
    for (jj = 0; jj < N; jj = jj + S) {
        /* 中间循环：按行分块 */
        for (kk = 0; kk < N; kk = kk + S) {
            /* 内层循环：处理当前块 */
            for (i = 0; i < N; i++) {
                for (j = jj; j < min(jj + S, N); j++) {
                    r = 0;
                    for (k = kk; k < min(kk + S, N); k++) {
                        r = r + y[i][k] * z[k][j];
                    }
                    x[i][j] = x[i][j] + r;
                }
            }
        }
    }
}

int main() {
    int n = 1024, i, j;
    float **x, **y, **z;
    double Total_time;
    clock_t start, end;

    printf("测试矩阵维数 n=%d\n", n);

    /* 分配内存 */
    y = (float**)malloc(n * sizeof(float*));
    z = (float**)malloc(n * sizeof(float*));
    x = (float**)malloc(n * sizeof(float*));
    for (i = 0; i < n; i++) {
        y[i] = (float*)malloc(n * sizeof(float));
        z[i] = (float*)malloc(n * sizeof(float));
        x[i] = (float*)malloc(n * sizeof(float));
    }

    /* 初始化矩阵 */
    for (i = 0; i < n; i++) {
        for (j = 0; j < n; j++) {
            y[i][j] = rand() % 10;
            z[i][j] = rand() % 10;
            x[i][j] = 0;
        }
    }

    /* 执行缓存分块矩阵乘法
     * 分块大小 S=16，需要根据目标架构的缓存大小调整 */
    start = clock();
    matrixmulti_blocked(n, (int**)x, (int**)y, (int**)z, 16);
    end = clock();
    Total_time = (double)(end - start) / CLOCKS_PER_SEC;
    printf("1024*1024 的矩阵乘缓存分块优化后：%lf秒\n", Total_time);

    /* 释放内存 */
    for (i = 0; i < n; i++) {
        free(y[i]);
        free(z[i]);
        free(x[i]);
    }
    free(y);
    free(z);
    free(x);

    return 0;
}

/**
 * 分块大小选择建议：
 * 1. S 应该使得 S×S 的块能够完全放入缓存
 * 2. 对于 L1 缓存（通常 32KB），S 可以选择 32-64
 * 3. 对于 L2 缓存（通常 256KB），S 可以选择 64-128
 * 4. 需要考虑数据类型大小（float 为 4 字节）
 *
 * 示例计算：
 * - L1 缓存 32KB，float 类型
 * - 每个块需要 3 * S * S * 4 字节（3 个矩阵）
 * - S = sqrt(32KB / (3 * 4)) ≈ 52
 * - 实际中通常选择 32 或 64
 *
 * 性能提升：
 * - 对于大矩阵，缓存分块可以显著减少缓存未命中
 * - 性能提升通常在 2-10 倍之间
 * - 具体提升取决于矩阵大小和缓存配置
 */
