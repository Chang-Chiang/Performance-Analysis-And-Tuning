/**
 * if 子句示例 - 条件并行
 *
 * 原理：
 * if 子句用于控制是否启用并行执行：
 * - 当条件为真时，使用多线程并行执行
 * - 当条件为假时，使用单线程串行执行
 *
 * 适用场景：
 * - 问题规模较小时，串行执行更快（避免线程创建开销）
 * - 问题规模较大时，并行执行更快
 *
 * 编译指令：
 * gcc -O2 -fopenmp -o if_clause if_clause.c
 *
 * 运行：
 * ./if_clause
 */

#include <stdio.h>
#include <omp.h>

#define N 2000
#define N_SMALL 100

float A[N][N], B[N][N];
float C[N][N];

/**
 * 示例 1：大矩阵 - 使用并行
 * 条件 N > 86 为真，启用并行
 */
void example1_large_matrix() {
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

    printf("=== 示例 1：大矩阵 (N=%d) - 使用并行 ===\n", N);
    start_time = omp_get_wtime();
    /* if(N > 86)：条件为真，使用 8 个线程 */
    #pragma omp parallel for private(i,j,k) shared(A,B,C) if(N>86) num_threads(8)
    for (i = 0; i < N; i++)
        for (j = 0; j < N; j++)
            for (k = 0; k < N; k++)
                C[i][j] += A[i][k] * B[k][j];
    end_time = omp_get_wtime();
    used_time = end_time - start_time;

    for (i = 0; i < N; i++)
        for (j = 0; j < N; j++)
            sum += C[i][j];
    printf("sum=%lf, used_time=%lf seconds\n\n", sum, used_time);
}

/**
 * 示例 2：小矩阵 - 使用串行
 * 条件 N_SMALL > 86 为假，使用单线程
 */
void example2_small_matrix() {
    float A_small[N_SMALL][N_SMALL], B_small[N_SMALL][N_SMALL], C_small[N_SMALL][N_SMALL];
    int i, j, k;
    double start_time, end_time, used_time;
    float sum = 0.0;

    /* 初始化小矩阵 */
    for (i = 0; i < N_SMALL; i++)
        for (j = 0; j < N_SMALL; j++) {
            A_small[i][j] = i + 1.0;
            B_small[i][j] = 1.0;
            C_small[i][j] = 0.0;
        }

    printf("=== 示例 2：小矩阵 (N=%d) - 使用串行 ===\n", N_SMALL);
    start_time = omp_get_wtime();
    /* if(N_SMALL > 86)：条件为假，使用单线程 */
    #pragma omp parallel for private(i,j,k) shared(A_small,B_small,C_small) if(N_SMALL>86) num_threads(8)
    for (i = 0; i < N_SMALL; i++)
        for (j = 0; j < N_SMALL; j++)
            for (k = 0; k < N_SMALL; k++)
                C_small[i][j] += A_small[i][k] * B_small[k][j];
    end_time = omp_get_wtime();
    used_time = end_time - start_time;

    for (i = 0; i < N_SMALL; i++)
        for (j = 0; j < N_SMALL; j++)
            sum += C_small[i][j];
    printf("sum=%lf, used_time=%lf seconds\n", sum, used_time);
}

int main() {
    example1_large_matrix();
    example2_small_matrix();

    return 0;
}

/**
 * if 子句详解：
 *
 * 语法：#pragma omp parallel if(condition)
 *
 * 参数：
 * - condition：条件表达式
 *   - 为真时：使用多线程并行执行
 *   - 为假时：使用单线程串行执行
 *
 * 工作原理：
 * 1. 编译器在运行时评估条件
 * 2. 如果条件为真，创建并行区
 * 3. 如果条件为假，串行执行代码
 *
 * 优点：
 * 1. 避免小问题的线程创建开销
 * 2. 自适应不同规模的问题
 * 3. 提高程序的整体性能
 *
 * 适用场景：
 * 1. 问题规模变化较大
 * 2. 线程创建开销相对较大
 * 3. 需要自适应并行度
 *
 * 注意事项：
 * 1. 条件应该在运行时评估
 * 2. 阈值需要根据实际情况调整
 * 3. 过度使用可能降低代码可读性
 */
