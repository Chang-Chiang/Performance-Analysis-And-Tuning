/**
 * 矩阵乘法 - 串行版本
 *
 * 原理：
 * 传统的三重循环矩阵乘法，用于 OpenMP 并行化的基准对比。
 *
 * 编译指令：
 * gcc -O2 -o matrix_multiply_serial matrix_multiply_serial.c
 *
 * 运行：
 * ./matrix_multiply_serial
 */

#include <stdio.h>
#include <omp.h>

#define N 2000

float A[N][N], B[N][N];
float C[N][N];

int main() {
    int i, j, k;
    float sum = 0.0;
    double start_time, end_time, used_time;

    /* 初始化矩阵 */
    for (i = 0; i < N; i++) {
        for (j = 0; j < N; j++) {
            A[i][j] = i + 1.0;
            B[i][j] = 1.0;
            C[i][j] = 0.0;
        }
    }

    /* 串行矩阵乘法 */
    start_time = omp_get_wtime();
    for (i = 0; i < N; i++)
        for (j = 0; j < N; j++)
            for (k = 0; k < N; k++)
                C[i][j] += A[i][k] * B[k][j];
    end_time = omp_get_wtime();
    used_time = end_time - start_time;

    /* 计算结果校验和 */
    for (i = 0; i < N; i++)
        for (j = 0; j < N; j++)
            sum += C[i][j];

    printf("串行版本：sum=%lf, used_time=%lf seconds\n", sum, used_time);

    return 0;
}

/**
 * 性能分析：
 * - 时间复杂度：O(N^3)
 * - 空间复杂度：O(N^2)
 * - 对于 N=2000，计算量约为 80 亿次浮点运算
 *
 * 并行化方向：
 * 1. 最外层循环（i）：每个线程计算 C 矩阵的若干行
 * 2. 中间层循环（j）：每个线程计算 C 矩阵的若干元素
 * 3. 内层循环（k）：不适合并行化，因为存在数据依赖
 */
