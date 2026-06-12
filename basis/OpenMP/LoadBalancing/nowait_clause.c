/**
 * nowait 子句示例 - 消除隐式屏障
 *
 * 原理：
 * nowait 子句用于消除 for 循环结束时的隐式屏障：
 * - 默认情况下，for 循环结束时有隐式屏障
 * - 使用 nowait 可以消除这个屏障
 * - 线程完成循环后可以立即继续执行
 *
 * 适用场景：
 * - 多个独立的 for 循环
 * - 循环之间没有数据依赖
 * - 需要减少同步开销
 *
 * 编译指令：
 * gcc -O2 -fopenmp -o nowait_clause nowait_clause.c
 *
 * 运行：
 * ./nowait_clause
 */

#include <stdio.h>
#include <omp.h>

#define N 2000

float A[N][N], B[N][N];
float C[N][N];

/**
 * 示例 1：有隐式屏障的并行循环
 * 每个 for 循环结束后都有隐式屏障
 */
void example1_with_barrier() {
    int i, j, k;
    double start_time, end_time, used_time;
    float sum = 0.0;

    /* 初始化矩阵 */
    for (i = 0; i < N; i++)
        for (j = 0; j < N; j++) {
            A[i][j] = i + 1.0;
            B[i][j] = 1.0;
            C[i][j] = 0.0;
        }

    printf("=== 示例 1：有隐式屏障 ===\n");
    start_time = omp_get_wtime();
    #pragma omp parallel private(i,j,k) shared(A,B,C) num_threads(4)
    {
        /* 第一个 for 循环：计算矩阵乘法
         * 隐式屏障：等待所有线程完成 */
        #pragma omp for
        for (i = 0; i < N; i++)
            for (j = 0; j < N; j++)
                for (k = 0; k < N; k++)
                    C[i][j] += A[i][k] * B[k][j];

        /* 隐式屏障在这里 */

        /* 第二个 for 循环：计算校验和
         * 隐式屏障：等待所有线程完成 */
        #pragma omp for reduction(+:sum)
        for (i = 0; i < N; i++)
            for (j = 0; j < N; j++)
                sum += C[i][j];
    }
    end_time = omp_get_wtime();
    used_time = end_time - start_time;

    printf("sum=%lf, used_time=%lf seconds\n\n", sum, used_time);
}

/**
 * 示例 2：使用 nowait 消除隐式屏障
 * 第一个 for 循环结束后不等待
 */
void example2_with_nowait() {
    int i, j, k;
    double start_time, end_time, used_time;
    float sum = 0.0;

    /* 重置矩阵 */
    for (i = 0; i < N; i++)
        for (j = 0; j < N; j++)
            C[i][j] = 0.0;

    printf("=== 示例 2：使用 nowait ===\n");
    start_time = omp_get_wtime();
    #pragma omp parallel private(i,j,k) shared(A,B,C) num_threads(4)
    {
        /* 第一个 for 循环：计算矩阵乘法
         * nowait：消除隐式屏障，线程可以立即继续 */
        #pragma omp for nowait
        for (i = 0; i < N; i++)
            for (j = 0; j < N; j++)
                for (k = 0; k < N; k++)
                    C[i][j] += A[i][k] * B[k][j];

        /* 没有隐式屏障，线程可以立即执行下一个 for */

        /* 第二个 for 循环：计算校验和 */
        #pragma omp for reduction(+:sum)
        for (i = 0; i < N; i++)
            for (j = 0; j < N; j++)
                sum += C[i][j];
    }
    end_time = omp_get_wtime();
    used_time = end_time - start_time;

    printf("sum=%lf, used_time=%lf seconds\n", sum, used_time);
}

int main() {
    example1_with_barrier();
    example2_with_nowait();

    return 0;
}

/**
 * nowait 子句详解：
 *
 * 语法：#pragma omp for nowait
 *
 * 工作原理：
 * 1. 默认情况下，for 循环结束时有隐式屏障
 * 2. 使用 nowait 消除这个屏障
 * 3. 线程完成循环后可以立即继续执行
 *
 * 优点：
 * 1. 减少同步开销
 * 2. 提高线程利用率
 * 3. 适用于独立的循环
 *
 * 缺点：
 * 1. 可能导致数据竞争（如果循环之间有依赖）
 * 2. 需要程序员确保数据一致性
 *
 * 适用场景：
 * 1. 多个独立的 for 循环
 * 2. 循环之间没有数据依赖
 * 3. 需要减少同步开销
 *
 * 注意事项：
 * 1. 确保循环之间没有数据依赖
 * 2. 使用 barrier 显式同步（如果需要）
 * 3. 使用 critical 或 atomic 保护共享数据
 *
 * 示例：正确的 nowait 使用
 * #pragma omp parallel
 * {
 *     #pragma omp for nowait
 *     for (i = 0; i < N; i++)
 *         A[i] = ...;  // 独立的计算
 *
 *     #pragma omp for
 *     for (i = 0; i < N; i++)
 *         B[i] = A[i] + ...;  // 依赖 A，需要同步
 * }
 */
