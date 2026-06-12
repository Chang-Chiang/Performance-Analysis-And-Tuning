/**
 * 标量替换优化示例 - 数组累加的寄存器重用
 *
 * 原理：
 * 在内层循环中对数组元素进行累加时，如果每次迭代都直接读写数组元素，
 * 会导致频繁的内存访问。通过引入标量变量（寄存器变量）来保存累加结果，
 * 可以显著减少内存访问次数。
 *
 * 优化前：
 * - 内层循环中每次迭代都访问数组 a[i]（读取和写入）
 * - 每次迭代需要 2 次内存访问（1 次读取 + 1 次写入）
 *
 * 优化后：
 * - 使用局部变量 sum 保存累加结果
 * - sum 会被编译器分配到寄存器中
 * - 内层循环结束后才写回数组，减少内存访问次数
 *
 * 编译指令：
 * gcc -O2 -o scalar_replacement_accumulation scalar_replacement_accumulation.c
 *
 * 运行：
 * ./scalar_replacement_accumulation
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <malloc.h>

int main() {
    int n = 10000, i, j;
    float *a, **b;
    clock_t start, end;
    double Total_time;

    /* 分配内存 */
    a = (float*)malloc(n * sizeof(float));
    b = (float**)malloc(n * sizeof(float*));
    for (i = 0; i < n; i++) {
        a[i] = 0;
        b[i] = (float*)malloc(n * sizeof(float));
    }

    /* 初始化数组 b */
    for (i = 0; i < n; i++) {
        for (j = 0; j < n; j++) {
            b[i][j] = rand() % 10;
        }
    }

    /* 优化前：每次迭代都访问数组 a */
    start = clock();
    for (i = 0; i < n; i++) {
        for (j = 0; j < n; j++) {
            a[i] = a[i] + b[i][j];  /* 每次迭代：读取 a[i] + 写入 a[i] */
        }
    }
    end = clock();
    Total_time = (double)(end - start) / CLOCKS_PER_SEC;
    printf("标量替换优化前：%lf秒\n", Total_time);

    /* 重置数组 a */
    for (i = 0; i < n; i++) a[i] = 0;

    /* 优化后：使用标量变量 sum 保存累加结果 */
    int sum;  /* 局部变量，编译器会将其分配到寄存器 */
    start = clock();
    for (i = 0; i < n; i++) {
        sum = a[i];  /* 从内存读取一次 */
        for (j = 0; j < n; j++) {
            sum = sum + b[i][j];  /* 在寄存器中累加 */
        }
        a[i] = sum;  /* 循环结束后写回内存 */
    }
    end = clock();
    Total_time = (double)(end - start) / CLOCKS_PER_SEC;
    printf("标量替换优化后：%lf秒\n", Total_time);

    /* 释放内存 */
    for (i = 0; i < n; i++) {
        free(b[i]);
    }
    free(a);
    free(b);
    return 0;
}

/**
 * 性能分析：
 * 优化前：内层循环 n 次迭代，每次 2 次内存访问 = 2n 次内存访问
 * 优化后：内层循环 n 次迭代，只有 1 次内存读取 + 1 次内存写入 = 2 次内存访问
 * 内存访问次数减少约 n 倍
 */
