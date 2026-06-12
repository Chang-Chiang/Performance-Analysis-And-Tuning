/**
 * for simd 指导语句示例
 *
 * 原理：
 * #pragma omp for simd 结合了工作共享和向量化：
 * - #pragma omp for：将循环迭代分配给多个线程
 * - #pragma omp simd：在每个线程内进行向量化
 *
 * 这样可以同时利用线程级并行和数据级并行：
 * - 线程级并行：多个线程同时处理不同的循环迭代
 * - 数据级并行：每个线程使用 SIMD 指令同时处理多个数据
 *
 * 编译指令：
 * gcc -O2 -fopenmp -msse4.2 -o for_simd_directive for_simd_directive.c
 *
 * 运行：
 * ./for_simd_directive
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

    /* 使用 for simd 进行矩阵乘法
     * #pragma omp parallel：创建并行区
     * #pragma omp for simd：工作共享 + 向量化
     * simdlen(8)：指定 SIMD 向量长度为 8 */
    start_time = omp_get_wtime();
    #pragma omp parallel num_threads(8)
    {
        #pragma omp for simd simdlen(8)
        for (int i = 0; i < N; i++)
            for (int j = 0; j < N; j++)
                for (int k = 0; k < N; k++)
                    C[i][j] += A[i][k] * B[k][j];
    }
    end_time = omp_get_wtime();
    used_time = end_time - start_time;

    /* 计算结果校验和 */
    for (i = 0; i < N; i++)
        for (j = 0; j < N; j++)
            sum += C[i][j];

    printf("for simd 版本：sum=%lf, used_time=%lf seconds\n", sum, used_time);

    return 0;
}

/**
 * for simd 指令详解：
 *
 * 语法：#pragma omp for simd [clause...]
 *
 * 子句：
 * - simdlen(N)：指定 SIMD 向量长度
 * - aligned(var:alignment)：指定数据对齐
 * - private(var)：私有变量
 * - reduction(operator:var)：归约操作
 * - schedule(type, chunk)：调度策略
 * - nowait：消除隐式屏障
 *
 * 执行流程：
 * 1. #pragma omp parallel：创建并行区，多个线程同时执行
 * 2. #pragma omp for：将循环迭代分配给多个线程
 * 3. #pragma omp simd：在每个线程内进行向量化
 * 4. 每个线程使用 SIMD 指令同时处理多个数据
 *
 * 性能提升：
 * - 假设 8 个线程，每个线程使用 8 路 SIMD
 * - 理论加速比：8 * 8 = 64 倍
 * - 实际加速比受内存带宽、缓存等因素限制
 *
 * 与单独使用 for 或 simd 的区别：
 * - #pragma omp for：只有线程级并行
 * - #pragma omp simd：只有数据级并行
 * - #pragma omp for simd：同时利用两种并行
 *
 * 注意事项：
 * 1. 循环迭代之间不能有数据依赖
 * 2. 数据访问模式应该适合向量化（连续访问）
 * 3. 使用 -O2 或 -O3 编译选项以启用向量化
 * 4. 使用 -msse4.2, -mavx, -mavx512f 等选项启用 SIMD 指令集
 */
