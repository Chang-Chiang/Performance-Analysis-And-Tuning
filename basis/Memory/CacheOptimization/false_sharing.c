/**
 * 伪共享问题示例
 *
 * 原理：
 * 伪共享（False Sharing）是指多个线程访问同一个缓存行中的不同变量，
 * 导致缓存行在多个核心之间频繁传输，降低性能。
 *
 * 本例展示了伪共享问题：
 * - 多个线程同时更新 sum 数组的不同元素
 * - 这些元素可能位于同一个缓存行中
 * - 导致缓存行在多个核心之间频繁传输
 *
 * 编译指令（需要 OpenMP 支持）：
 * gcc -O2 -fopenmp -o false_sharing false_sharing.c
 *
 * 运行：
 * ./false_sharing
 */

#include <omp.h>

#define N 100000000
#define THREAD_NUM 8

/* 全局数组 */
int values[N];

int main(void) {
    int sum[THREAD_NUM];

    /* 初始化数组 */
    for (int i = 0; i < N; i++) {
        values[i] = 1;
    }

    /* 并行计算
     * 问题：sum 数组的元素可能位于同一个缓存行中
     * 每个线程更新自己的 sum[i] 时，会导致其他线程的缓存行失效 */
    #pragma omp parallel for
    for (int i = 0; i < THREAD_NUM; i++) {
        sum[i] = 0;
        for (int j = 0; j < N; j++) {
            sum[i] += values[j] >> i;
        }
    }

    /* 打印结果 */
    for (int i = 0; i < THREAD_NUM; i++) {
        printf("sum[%d] = %d\n", i, sum[i]);
    }

    return 0;
}

/**
 * 伪共享的影响：
 * 1. 缓存行在多个核心之间频繁传输
 * 2. 增加内存总线流量
 * 3. 降低程序性能
 *
 * 解决方案：
 * 1. 使用填充（Padding）确保每个线程的数据位于不同的缓存行
 * 2. 使用局部变量保存中间结果，最后再写回共享数组
 * 3. 使用 threadprivate 或 reduction 子句
 *
 * 缓存行大小：
 * - 现代 CPU 的缓存行通常为 64 字节
 * - int 类型通常为 4 字节
 * - 一个缓存行可以容纳 16 个 int 变量
 */
