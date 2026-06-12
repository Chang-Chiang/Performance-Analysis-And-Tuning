/**
 * 矩阵乘法 - 缓存分块优化前
 *
 * 原理：
 * 传统的矩阵乘法按行和列遍历矩阵，会导致频繁的缓存未命中。
 * 本例展示了优化前的矩阵乘法，作为缓存分块优化的对比基准。
 *
 * 问题分析：
 * - 内层循环遍历矩阵 z 的列，导致非连续内存访问
 * - 每次内层循环迭代都可能产生缓存未命中
 * - 对于大矩阵，性能会显著下降
 *
 * 编译指令：
 * gcc -O2 -o matrix_multiply matrix_multiply.c
 *
 * 运行：
 * ./matrix_multiply
 */

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <time.h>

/**
 * 传统矩阵乘法
 * @param N 矩阵维度
 * @param x 结果矩阵
 * @param y 输入矩阵 1
 * @param z 输入矩阵 2
 *
 * 复杂度：O(N^3)
 * 缓存行为：内层循环访问 z[k][j]，列访问导致缓存未命中
 */
void matrixmulti(int N, float** x, float** y, float** z) {
    int i, j, k;
    for (i = 0; i < N; i++) {
        for (j = 0; j < N; j++) {
            float r = 0;
            for (k = 0; k < N; k++) {
                r = r + y[i][k] * z[k][j];  /* z[k][j] 是列访问 */
            }
            x[i][j] = r;
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

    /* 执行矩阵乘法 */
    start = clock();
    matrixmulti(n, x, y, z);
    end = clock();
    Total_time = (double)(end - start) / CLOCKS_PER_SEC;
    printf("1024*1024 的矩阵乘缓存分块优化前：%lf秒\n", Total_time);

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
 * 性能分析：
 * 1. 内层循环访问 z[k][j] 是列访问，每次访问都可能产生缓存未命中
 * 2. 对于 N=1024 的矩阵，缓存未命中率很高
 * 3. 性能瓶颈在于内存访问模式，而不是计算
 *
 * 优化方向：
 * 1. 使用缓存分块（Cache Blocking）技术
 * 2. 重新组织循环顺序，使内存访问更加连续
 * 3. 使用分块矩阵乘法，减少缓存未命中
 */
