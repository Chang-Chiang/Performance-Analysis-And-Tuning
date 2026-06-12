/**
 * SIMD 指导语句示例
 *
 * 原理：
 * SIMD（Single Instruction Multiple Data）是一种数据级并行技术，
 * 可以同时对多个数据执行相同的操作。
 *
 * OpenMP 提供了 #pragma omp simd 指令来帮助编译器进行向量化：
 * - 提示编译器将循环向量化
 * - 使用 SIMD 指令同时处理多个数据
 * - 提高计算密集型任务的性能
 *
 * 编译指令：
 * gcc -O2 -fopenmp -msse4.2 -o simd_directive simd_directive.c
 *
 * 运行：
 * ./simd_directive
 */

#include <stdio.h>
#include <omp.h>

#define N 2000

float A[N][N], B[N][N];
float C[N][N];

int main() {
    int i, j, k;
    double start_time, end_time, used_time;
    float sum = 0.0;

    /* 初始化矩阵 */
    for (i = 0; i < N; i++) {
        for (j = 0; j < N; j++) {
            A[i][j] = i + 1.0;
            B[i][j] = 1.0;
            C[i][j] = 0.0;
        }
    }

    /* 使用 SIMD 指令进行矩阵乘法
     * #pragma omp simd：提示编译器将循环向量化
     * 编译器会尝试使用 SIMD 指令同时处理多个数据 */
    start_time = omp_get_wtime();
    #pragma omp simd
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            for (int k = 0; k < N; k++)
                C[i][j] += A[i][k] * B[k][j];
    end_time = omp_get_wtime();
    used_time = end_time - start_time;

    /* 计算结果校验和 */
    for (i = 0; i < N; i++)
        for (j = 0; j < N; j++)
            sum += C[i][j];

    printf("SIMD 版本：sum=%lf, used_time=%lf seconds\n", sum, used_time);

    return 0;
}

/**
 * SIMD 指令详解：
 *
 * 1. #pragma omp simd：
 *    - 提示编译器将循环向量化
 *    - 不创建并行区，只进行向量化
 *    - 适用于最内层循环
 *
 * 2. SIMD 子句：
 *    - simdlen(N)：指定 SIMD 向量长度
 *    - aligned(var:alignment)：指定数据对齐
 *    - private(var)：私有变量
 *    - reduction(operator:var)：归约操作
 *
 * 3. 向量化条件：
 *    - 循环迭代之间没有数据依赖
 *    - 循环体内的操作可以并行执行
 *    - 数据访问模式适合向量化（连续访问）
 *
 * 4. 性能提升：
 *    - SSE：128 位，同时处理 4 个 float 或 2 个 double
 *    - AVX：256 位，同时处理 8 个 float 或 4 个 double
 *    - AVX-512：512 位，同时处理 16 个 float 或 8 个 double
 *
 * 注意事项：
 * 1. 编译器可能忽略 SIMD 指令（如果条件不满足）
 * 2. 使用 -O2 或 -O3 编译选项以启用向量化
 * 3. 使用 -msse4.2, -mavx, -mavx512f 等选项启用 SIMD 指令集
 */
