/**
 * 内存访问优化示例 - 缓存行对齐
 *
 * 原理：
 * 通过调整数组大小使其对齐到缓存行边界，可以提高缓存利用率。
 * 本例展示了不同矩阵大小对矩阵乘法性能的影响。
 *
 * 优化前：
 * - 矩阵大小为 251，不是缓存行大小的倍数
 * - 导致缓存行利用率低
 *
 * 优化后：
 * - 矩阵大小扩展到 256，是缓存行大小的倍数
 * - 提高缓存行利用率，减少缓存未命中
 *
 * 编译指令：
 * gcc -O2 -o memory_access_optimization memory_access_optimization.c
 *
 * 运行：
 * ./memory_access_optimization
 */

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <time.h>

/* 缓存行大小（字节） */
#define Cache_length 64

int main() {
    int n = 251, i, j, k;
    clock_t start, end;
    double Total_time;
    int Cache_len = 256;  /* 扩展后的大小，是缓存行大小的倍数 */

    float **a, **b, **c, **a1, **b1, **c1;

    /* 分配内存 - 原始大小 */
    a = (float**)malloc(n * sizeof(float*));
    b = (float**)malloc(n * sizeof(float*));
    c = (float**)malloc(n * sizeof(float*));

    /* 分配内存 - 扩展大小 */
    a1 = (float**)malloc(Cache_len * sizeof(float*));
    b1 = (float**)malloc(Cache_len * sizeof(float*));
    c1 = (float**)malloc(Cache_len * sizeof(float*));

    for (i = 0; i < n; i++) {
        a[i] = (float*)malloc(n * sizeof(float));
        b[i] = (float*)malloc(n * sizeof(float));
        c[i] = (float*)malloc(n * sizeof(float));
    }

    for (i = 0; i < Cache_len; i++) {
        a1[i] = (float*)malloc(Cache_len * sizeof(float));
        b1[i] = (float*)malloc(Cache_len * sizeof(float));
        c1[i] = (float*)malloc(Cache_len * sizeof(float));
    }

    /* 初始化矩阵 */
    for (i = 0; i < n; i++) {
        for (j = 0; j < n; j++) {
            a[i][j] = rand() % 10;
            b[i][j] = rand() % 10;
            c[i][j] = 0;
        }
    }

    for (i = 0; i < Cache_len; i++) {
        for (j = 0; j < Cache_len; j++) {
            a1[i][j] = rand() % 10;
            b1[i][j] = rand() % 10;
            c1[i][j] = 0;
        }
    }

    /* 测试 1：扩展大小的矩阵乘法 */
    start = clock();
    for (i = 0; i < Cache_len; i++) {
        for (j = 0; j < Cache_len; j++) {
            c1[i][j] = 0;
            for (k = 0; k < Cache_len; k++) {
                c1[i][j] += a1[i][k] * b1[k][j];
            }
        }
    }
    end = clock();
    Total_time = (double)(end - start) / CLOCKS_PER_SEC;
    printf("256*256 普通矩阵乘耗时：%lf秒\n", Total_time);

    /* 测试 2：原始大小的矩阵乘法 */
    start = clock();
    for (i = 0; i < n; i++) {
        for (j = 0; j < n; j++) {
            c[i][j] = 0;
            for (k = 0; k < n; k++) {
                c[i][j] += a[i][k] * b[k][j];
            }
        }
    }
    end = clock();
    Total_time = (double)(end - start) / CLOCKS_PER_SEC;
    printf("251*251 普通矩阵乘耗时：%lf秒\n", Total_time);

    /* 测试 3：列扩展后的矩阵乘法
     * 将 251x251 的矩阵扩展到 251x256，使列数对齐到缓存行 */
    for (i = 0; i < n; i++) {
        a[i] = (float*)realloc(a[i], sizeof(float) * Cache_len);
        b[i] = (float*)realloc(b[i], sizeof(float) * Cache_len);
    }
    for (i = 0; i < n; i++) {
        for (j = n; j < Cache_len; j++) {
            a[i][j] = 0;
            b[i][j] = 0;
        }
    }

    start = clock();
    for (i = 0; i < n; i++) {
        for (j = 0; j < n; j++) {
            c[i][j] = 0;
            for (k = 0; k < n; k++) {
                c[i][j] += a[i][k] * b[k][j];
            }
        }
    }
    end = clock();
    Total_time = (double)(end - start) / CLOCKS_PER_SEC;
    printf("列扩展后的矩阵乘耗时：%lf秒\n", Total_time);

    /* 释放内存 */
    for (i = 0; i < n; i++) {
        free(a[i]);
        free(b[i]);
        free(c[i]);
    }
    for (i = 0; i < Cache_len; i++) {
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

/**
 * 优化原理：
 * 1. 缓存行通常为 64 字节，可以容纳 16 个 float 类型数据
 * 2. 当矩阵列数是 16 的倍数时，每行数据恰好占满整数个缓存行
 * 3. 这样可以提高缓存行的利用率，减少缓存未命中
 *
 * 优化方法：
 * 1. 扩展矩阵列数到缓存行大小的倍数
 * 2. 填充的元素初始化为 0，不影响计算结果
 * 3. 对于行优先存储的矩阵，优化列数更重要
 *
 * 性能提升：
 * - 对于 251x251 的矩阵，扩展到 256x256 可以显著提高性能
 * - 具体提升取决于缓存大小和矩阵大小
 * - 通常可以提升 10-30% 的性能
 */
