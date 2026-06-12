/**
 * OpenMP parallel for 指令详解
 *
 * 原理：
 * 展示 #pragma omp parallel for 指令的各种用法，
 * 包括变量作用域、线程同步等基本概念。
 *
 * 编译指令：
 * gcc -O2 -fopenmp -o parallel_for_basics parallel_for_basics.c
 *
 * 运行：
 * ./parallel_for_basics
 */

#include <stdio.h>
#include <omp.h>

#define N 16

/**
 * 示例 1：基本的 parallel for
 * 展示并行循环的基本用法
 */
void example1_basic_parallel_for() {
    int A[N];

    printf("=== 示例 1：基本的 parallel for ===\n");
    #pragma omp parallel for private(i) num_threads(4)
    for (int i = 0; i < N; i++) {
        A[i] = i;
        printf("thread %d: A[%d] = %d\n", omp_get_thread_num(), i, A[i]);
    }

    /* 主线程输出完成信息 */
    #pragma omp master
    printf("所有工作完成！\n\n");
}

/**
 * 示例 2：函数中的 parallel for
 * 展示在函数中使用 OpenMP 指令
 */
void init_array_parallel(int* a, int n) {
    #pragma omp parallel for
    for (int i = 0; i < n; i++) {
        a[i] = i;
    }
}

/**
 * 示例 3：函数中的 omp for
 * 展示 omp for 与 parallel for 的区别
 */
void init_array_omp_for(int* b, int n) {
    /* 注意：这里使用 omp for 而不是 parallel for
     * 需要在 parallel 区域内使用 */
    #pragma omp for
    for (int i = 0; i < n; i++) {
        b[i] = i;
    }
}

int main() {
    double start_time, end_time, used_time;
    int B[N];

    /* 示例 1 */
    example1_basic_parallel_for();

    /* 示例 2 */
    printf("=== 示例 2：函数中的 parallel for ===\n");
    start_time = omp_get_wtime();
    init_array_parallel(B, N);
    end_time = omp_get_wtime();
    used_time = end_time - start_time;
    printf("used_time = %lf seconds\n\n", used_time);

    /* 示例 3 */
    printf("=== 示例 3：函数中的 omp for ===\n");
    start_time = omp_get_wtime();
    #pragma omp parallel
    init_array_omp_for(B, N);
    end_time = omp_get_wtime();
    used_time = end_time - start_time;
    printf("used_time = %lf seconds\n", used_time);

    return 0;
}

/**
 * parallel for vs omp for 的区别：
 *
 * 1. #pragma omp parallel for：
 *    - 创建并行区 + 工作共享
 *    - 等价于 #pragma omp parallel + #pragma omp for
 *    - 简洁，适合简单场景
 *
 * 2. #pragma omp for：
 *    - 只进行工作共享，不创建并行区
 *    - 必须在 parallel 区域内使用
 *    - 更灵活，可以在同一个并行区内执行多个 for 循环
 *
 * 变量作用域：
 * - private：每个线程有独立的副本
 * - shared：所有线程共享同一个变量
 * - firstprivate：私有变量，但初始化为主线程的值
 * - reduction：归约操作，每个线程有私有副本，最后合并
 */
