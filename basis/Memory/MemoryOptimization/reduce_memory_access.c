/**
 * 减少内存读写优化示例
 *
 * 原理：
 * 通过保存中间计算结果到寄存器中，减少对内存的读写次数。
 * 特别是在循环中，如果同一个内存位置被频繁读写，
 * 可以通过引入临时变量来减少内存访问。
 *
 * 优化前：
 * - 每次迭代都从内存读取 a[i-1]，然后写入 a[i]
 * - 存在数据依赖，后一次循环需要使用前一次循环的结果
 *
 * 优化后：
 * - 使用临时变量 temp 保存中间结果
 * - 减少了一次内存读取操作
 *
 * 编译指令：
 * gcc -O2 -o reduce_memory_access reduce_memory_access.c
 *
 * 运行：
 * ./reduce_memory_access
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <malloc.h>

int main() {
    int n = 100000000, i;
    float *a, *b, temp;
    clock_t start, end;
    double Total_time;

    /* 分配内存 */
    a = (float*)malloc(n * sizeof(float));
    b = (float*)malloc(n * sizeof(float));

    /* 初始化数组 */
    for (i = 0; i < n; i++) {
        a[i] = rand() % 10;
        b[i] = rand() % 10;
    }

    /* 优化前：每次迭代都访问内存 */
    start = clock();
    for (i = 1; i < n; i++) {
        /* 问题：每次迭代都需要：
         * 1. 从内存读取 a[i-1]
         * 2. 从内存读取 a[i]
         * 3. 计算 a[i] + a[i-1]
         * 4. 将结果写回 a[i] */
        a[i] += a[i - 1];
    }
    end = clock();
    Total_time = (double)(end - start) / CLOCKS_PER_SEC;
    printf("减少内存读写优化前：%lf秒\n", Total_time);

    /* 重置数组 a */
    for (i = 0; i < n; i++) {
        a[i] = rand() % 10;
    }

    /* 优化后：使用临时变量减少内存访问 */
    start = clock();
    temp = a[0];  /* 保存初始值到寄存器 */
    for (i = 1; i < n; i++) {
        /* 优化：
         * 1. temp 保存在寄存器中，不需要从内存读取
         * 2. 只需要从内存读取 a[i]
         * 3. 计算 temp + a[i]
         * 4. 将结果写回 a[i] 和 temp */
        temp += a[i];
        a[i] = temp;
    }
    end = clock();
    Total_time = (double)(end - start) / CLOCKS_PER_SEC;
    printf("减少内存读写优化后：%lf秒\n", Total_time);

    /* 释放内存 */
    free(a);
    free(b);
    return 0;
}

/**
 * 性能分析：
 * 优化前：每次迭代 2 次内存读取 + 1 次内存写入 = 3 次内存访问
 * 优化后：每次迭代 1 次内存读取 + 1 次内存写入 = 2 次内存访问
 * 内存访问次数减少 33%
 *
 * 注意事项：
 * 1. 这种优化适用于存在数据依赖的循环
 * 2. 临时变量 temp 应该保存在寄存器中
 * 3. 编译器通常会自动进行这种优化，但手动优化可以确保效果
 * 4. 对于没有数据依赖的循环，这种优化效果不明显
 */
