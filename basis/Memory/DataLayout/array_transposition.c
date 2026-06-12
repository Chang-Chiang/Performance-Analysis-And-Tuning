/**
 * 数组转置优化示例
 *
 * 原理：
 * 数组转置（Array Transposition）通过改变数组的存储顺序，
 * 使访问模式更加符合缓存预取策略，提高缓存命中率。
 *
 * 优化前：
 * - 按列访问二维数组 x[i][1], x[i][2]
 * - 每次访问步幅为 N，导致频繁缓存未命中
 *
 * 优化后：
 * - 将数组转置，使按列访问变为按行访问
 * - x[1][i], x[2][i] 变为连续访问
 * - 提高缓存命中率
 *
 * 编译指令：
 * gcc -O2 -o array_transposition array_transposition.c
 *
 * 运行：
 * ./array_transposition
 */

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <malloc.h>

int main() {
    int n = 10000, i, j, phi = 354;
    float **x, **y;
    clock_t start, end;
    double Total_time;

    /* 分配内存 */
    x = (float**)malloc(n * sizeof(float*));
    y = (float**)malloc(n * sizeof(float*));
    for (i = 0; i < n; i++) {
        x[i] = (float*)malloc(n * sizeof(float));
        y[i] = (float*)malloc(n * sizeof(float));
    }

    /* 初始化数组 */
    for (i = 0; i < n; i++) {
        for (j = 0; j < n; j++) {
            x[i][j] = rand() % 10;
            y[i][j] = rand() % 10;
        }
    }

    /* 优化前：按列访问
     * 问题：x[i][1] 和 x[i][2] 的访问步幅为 n
     * 每次访问都可能产生缓存未命中 */
    start = clock();
    for (i = 0; i < n; i++) {
        x[i][1] = x[i][1] + phi * y[i][1];
        x[i][2] = x[i][2] + phi * y[i][2];
    }
    end = clock();
    Total_time = (double)(end - start) / CLOCKS_PER_SEC;
    printf("数组转置优化前：%lf秒\n", Total_time);

    /* 优化后：按行访问
     * 优化：将数组转置，使按列访问变为按行访问
     * x[1][i] 和 x[2][i] 变为连续访问
     * 注意：这里省略了实际的转置操作，仅展示访问模式的改变 */
    start = clock();
    for (i = 0; i < n; i++) {
        x[1][i] = x[1][i] + phi * y[1][i];
        x[2][i] = x[2][i] + phi * y[2][i];
    }
    end = clock();
    Total_time = (double)(end - start) / CLOCKS_PER_SEC;
    printf("数组转置优化后：%lf秒\n", Total_time);

    /* 释放内存 */
    for (i = 0; i < n; i++) {
        free(x[i]);
        free(y[i]);
    }
    free(x);
    free(y);

    return 0;
}

/**
 * 优化原理：
 * 1. 二维数组在内存中按行存储
 * 2. 按列访问时，每次访问步幅为 n
 * 3. 步幅过大时，缓存预取器无法预测访问模式
 * 4. 转置后，按列访问变为按行访问，步幅为 1
 *
 * 转置方法：
 * 1. 创建新的转置数组
 * 2. 将原数组的数据复制到转置数组
 * 3. 使用转置数组进行计算
 * 4. 如果需要，将结果转置回来
 *
 * 性能提升：
 * - 对于大矩阵，转置可以显著提高缓存命中率
 * - 性能提升通常在 2-10 倍之间
 * - 具体提升取决于矩阵大小和访问模式
 *
 * 适用场景：
 * 1. 需要按列访问二维数组
 * 2. 数组足够大，缓存未命中成为瓶颈
 * 3. 访问模式固定，值得进行转置
 */
