/**
 * 伪共享分析示例
 *
 * 原理：
 * 伪共享（False Sharing）是指多个线程访问同一个缓存行中的不同变量，
 * 导致缓存行在多个核心之间频繁传输，降低性能。
 *
 * 本例通过计算 π 值来展示伪共享问题：
 * - 串行版本：单线程计算
 * - 伪共享版本：多线程使用共享数组，但数组元素在同一缓存行中
 *
 * 编译指令：
 * gcc -O2 -fopenmp -o false_sharing_analysis false_sharing_analysis.c
 *
 * 运行：
 * ./false_sharing_analysis
 */

#include <stdio.h>
#include <omp.h>

#define NTHREADS 4
static long num_steps = 1000000;
double step;

/**
 * 串行版本：单线程计算 π
 * 作为性能基准
 */
void pi_serial() {
    int i;
    double x, pi, sum = 0.0;
    double start_time, end_time;

    step = 1.0 / (double)num_steps;

    start_time = omp_get_wtime();
    for (i = 0; i < num_steps; i++) {
        x = (i + 0.5) * step;
        sum += 4.0 / (1.0 + x * x);
    }
    pi = step * sum;
    end_time = omp_get_wtime();

    printf("串行版本：pi = %15.13f, runtime = %lf seconds\n", pi, end_time - start_time);
}

/**
 * 伪共享版本：多线程使用共享数组
 * 问题：sum 数组的元素可能在同一缓存行中
 *
 * 缓存行大小通常为 64 字节
 * double 类型为 8 字节
 * 一个缓存行可以容纳 8 个 double 变量
 * sum[0] 和 sum[1] 可能在同一缓存行中！
 */
void pi_false_sharing() {
    int i, j;
    double pi, start_time, used_time;
    double sum[NTHREADS] = {0.0};  /* 共享数组，可能在同一缓存行 */

    step = 1.0 / (double)num_steps;

    start_time = omp_get_wtime();
    #pragma omp parallel num_threads(NTHREADS)
    {
        int i;
        int id = omp_get_thread_num();
        double x;

        /* 每个线程更新自己的 sum[id]
         * 问题：sum[id] 可能与其他线程的 sum 在同一缓存行 */
        #pragma omp for
        for (i = 0; i < num_steps; i++) {
            x = (i + 0.5) * step;
            sum[id] += 4.0 / (1.0 + x * x);
        }
    }

    /* 汇总结果 */
    pi = 0.0;
    for (i = 0; i < NTHREADS; i++)
        pi += sum[i];
    pi = step * pi;

    used_time = omp_get_wtime() - start_time;
    printf("伪共享版本：pi = %15.13f, runtime = %lf seconds\n", pi, used_time);
}

int main() {
    printf("=== 伪共享分析 ===\n");
    pi_serial();
    pi_false_sharing();

    return 0;
}

/**
 * 伪共享问题分析：
 *
 * 1. 缓存行大小：
 *    - 现代 CPU 的缓存行通常为 64 字节
 *    - double 类型为 8 字节
 *    - 一个缓存行可以容纳 8 个 double 变量
 *
 * 2. 问题描述：
 *    - sum[0], sum[1], sum[2], sum[3] 可能在同一缓存行中
 *    - 当线程 0 更新 sum[0] 时，整个缓存行被标记为无效
 *    - 线程 1 访问 sum[1] 时，需要重新加载缓存行
 *    - 这导致缓存行在多个核心之间频繁传输
 *
 * 3. 性能影响：
 *    - 增加内存总线流量
 *    - 降低程序性能
 *    - 多线程版本可能比串行版本更慢
 *
 * 解决方案：
 * 1. 数据填充：在数组元素之间添加填充，确保不同线程的数据在不同缓存行
 * 2. 数据私有化：使用 reduction 子句，每个线程有私有副本
 */
