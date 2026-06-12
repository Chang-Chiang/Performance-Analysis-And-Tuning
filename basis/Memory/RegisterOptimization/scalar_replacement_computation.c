/**
 * 标量替换优化示例 - 数组计算的寄存器重用
 *
 * 原理：
 * 在循环中对多个数组进行计算时，如果同一个数组元素被多次读取，
 * 可以通过引入标量变量来保存读取结果，减少重复的内存访问。
 *
 * 优化前：
 * - 循环中每次迭代都读取 a[i] 和 b[i] 两次
 * - 每次迭代需要 4 次内存读取
 *
 * 优化后：
 * - 使用局部变量 x, y 保存 a[i] 和 b[i] 的值
 * - x, y 会被编译器分配到寄存器中
 * - 每次迭代只需要 2 次内存读取
 *
 * 编译指令：
 * gcc -O2 -o scalar_replacement_computation scalar_replacement_computation.c
 *
 * 运行：
 * ./scalar_replacement_computation
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <malloc.h>

int main() {
    int n = 100000000, i;
    float *a, *b, *c, *d;
    double Total_time;
    clock_t start, end;

    /* 分配内存 */
    a = (float*)malloc(n * sizeof(float));
    b = (float*)malloc(n * sizeof(float));
    c = (float*)malloc(n * sizeof(float));
    d = (float*)malloc(n * sizeof(float));

    /* 初始化数组 */
    for (i = 0; i < n; i++) {
        a[i] = rand() % 10;
        b[i] = rand() % 10;
    }

    /* 优化前：每次迭代都读取 a[i] 和 b[i] */
    start = clock();
    for (i = 0; i < n; i++) {
        c[i] = a[i] + b[i];  /* 读取 a[i], b[i] */
        d[i] = a[i] - b[i];  /* 再次读取 a[i], b[i] */
    }
    end = clock();
    Total_time = (double)(end - start) / CLOCKS_PER_SEC;
    printf("标量替换优化前：%lf秒\n", Total_time);

    /* 优化后：使用标量变量保存读取结果 */
    int x, y;  /* 局部变量，编译器会将其分配到寄存器 */
    start = clock();
    for (i = 0; i < n; i++) {
        x = a[i];      /* 读取一次 a[i]，保存到寄存器 */
        y = b[i];      /* 读取一次 b[i]，保存到寄存器 */
        c[i] = x + y;  /* 从寄存器读取 x, y */
        d[i] = x - y;  /* 从寄存器读取 x, y */
    }
    end = clock();
    Total_time = (double)(end - start) / CLOCKS_PER_SEC;
    printf("标量替换优化后：%lf秒\n", Total_time);

    /* 释放内存 */
    free(a);
    free(b);
    free(c);
    free(d);
    return 0;
}

/**
 * 性能分析：
 * 优化前：每次迭代 4 次内存读取（a[i] 两次, b[i] 两次）
 * 优化后：每次迭代 2 次内存读取（a[i] 一次, b[i] 一次）
 * 内存读取次数减少 50%
 */
