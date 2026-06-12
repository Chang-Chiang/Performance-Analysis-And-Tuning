/**
 * 矩阵乘法 - OpenMP 并行版本
 *
 * 原理：
 * 使用 OpenMP 将矩阵乘法的最外层循环并行化，
 * 多个线程同时计算 C 矩阵的不同行。
 *
 * 编译指令：
 * gcc -O2 -fopenmp -o matrix_multiply_parallel matrix_multiply_parallel.c
 *
 * 运行：
 * ./matrix_multiply_parallel
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

    /* OpenMP 并行矩阵乘法
     * #pragma omp parallel for：将最外层循环并行化
     * private(i,j,k)：循环变量为私有变量
     * shared(A,B,C)：矩阵为共享变量
     * num_threads(4)：使用 4 个线程 */
    start_time = omp_get_wtime();
    #pragma omp parallel for private(i,j,k) shared(A,B,C) num_threads(4)
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

    printf("并行版本：sum=%lf, used_time=%lf seconds\n", sum, used_time);

    return 0;
}

/**
 * 并行化分析：
 *
 * 1. 并行区域：
 *    - 最外层循环（i 循环）被并行化
 *    - 每个线程计算 C 矩阵的 N/num_threads 行
 *
 * 2. 数据依赖：
 *    - C[i][j] 的计算只依赖于 A[i][k] 和 B[k][j]
 *    - 不同的 i 值之间没有数据依赖
 *    - 因此可以安全地并行化
 *
 * 3. 性能提升：
 *    - 理想情况下，4 个线程可以获得接近 4 倍的加速
 *    - 实际加速比受内存带宽、缓存等因素影响
 *
 * 4. 变量属性：
 *    - private(i,j,k)：每个线程有独立的循环变量
 *    - shared(A,B,C)：所有线程共享矩阵数据
 */
