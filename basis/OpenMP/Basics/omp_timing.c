/**
 * OpenMP 计时函数示例
 *
 * 原理：
 * 展示如何使用 OpenMP 的计时函数 omp_get_wtime()
 * 来测量程序的执行时间。
 *
 * 编译指令：
 * gcc -O2 -fopenmp -o omp_timing omp_timing.c
 *
 * 运行：
 * ./omp_timing
 */

#include <stdio.h>
#include <omp.h>

static long num_steps = 1000000;
double step;

int main() {
    int i;
    double x, pi, sum = 0.0;
    double start_time, end_time;

    step = 1.0 / (double)num_steps;

    /* 开始计时 */
    start_time = omp_get_wtime();

    /* 计算 π 值（串行版本） */
    for (i = 0; i < num_steps; i++) {
        x = (i + 0.5) * step;
        sum += 4.0 / (1.0 + x * x);
    }
    pi = step * sum;

    /* 结束计时 */
    end_time = omp_get_wtime();

    printf("pi = %15.13f, runtime = %lf seconds\n", pi, end_time - start_time);

    return 0;
}

/**
 * omp_get_wtime() 函数：
 *
 * 功能：返回自某个固定时间点以来的秒数
 * 精度：通常为微秒级别
 * 用途：测量代码段的执行时间
 *
 * 使用方法：
 * double start = omp_get_wtime();
 * // 要测量的代码
 * double end = omp_get_wtime();
 * double elapsed = end - start;
 *
 * 注意事项：
 * 1. 这是一个墙钟时间（wall clock time），不是 CPU 时间
 * 2. 在并行程序中，墙钟时间更能反映实际性能
 * 3. 精度取决于系统实现
 */
