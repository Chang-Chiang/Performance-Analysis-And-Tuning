/**
 * 伪共享问题解决方案示例
 *
 * 原理：
 * 通过使用局部变量保存中间计算结果，避免多个线程同时访问
 * 同一个缓存行中的不同变量，从而解决伪共享问题。
 *
 * 优化前：
 * - 多个线程同时更新 sum 数组的不同元素
 * - 这些元素可能位于同一个缓存行中
 * - 导致伪共享问题
 *
 * 优化后：
 * - 每个线程使用局部变量 local_sum 保存中间结果
 * - 计算完成后才写回 sum 数组
 * - 避免了伪共享问题
 *
 * 编译指令（需要 OpenMP 支持）：
 * gcc -O2 -fopenmp -o false_sharing_solution false_sharing_solution.c
 *
 * 运行：
 * ./false_sharing_solution
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

    /* 并行计算 - 使用局部变量避免伪共享 */
    #pragma omp parallel for
    for (int i = 0; i < THREAD_NUM; i++) {
        int local_sum = 0;  /* 局部变量，保存在寄存器或栈上 */

        /* 使用局部变量进行累加 */
        for (int j = 0; j < N; j++) {
            local_sum += values[j] >> i;
        }

        /* 计算完成后写回共享数组 */
        sum[i] = local_sum;
    }

    /* 打印结果 */
    for (int i = 0; i < THREAD_NUM; i++) {
        printf("sum[%d] = %d\n", i, sum[i]);
    }

    return 0;
}

/**
 * 优化效果：
 * 1. 局部变量 local_sum 保存在寄存器或线程私有的栈上
 * 2. 内层循环中不会访问共享的 sum 数组
 * 3. 避免了缓存行在多个核心之间的频繁传输
 * 4. 显著提高了程序性能
 *
 * 其他解决方案：
 * 1. 使用填充（Padding）确保每个线程的数据位于不同的缓存行
 *    int sum[THREAD_NUM][8];  // 8 * sizeof(int) = 32 字节，确保不同缓存行
 *
 * 2. 使用 aligned 属性确保数据对齐
 *    int sum[THREAD_NUM] __attribute__((aligned(64)));
 *
 * 3. 使用 reduction 子句
 *    #pragma omp parallel for reduction(+:sum[:THREAD_NUM])
 */
