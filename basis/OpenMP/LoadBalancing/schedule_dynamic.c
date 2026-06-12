/**
 * dynamic 调度策略示例
 *
 * 原理：
 * dynamic 调度策略将循环迭代动态分配给线程：
 * - 迭代块在运行时分配给空闲线程
 * - 线程完成一个块后立即请求下一个块
 * - 适用于迭代计算量不均匀的场景
 *
 * 语法：
 * #pragma omp for schedule(dynamic)      // 默认块大小 = 1
 * #pragma omp for schedule(dynamic, 10)  // 块大小为 10
 *
 * 编译指令：
 * gcc -O2 -fopenmp -o schedule_dynamic schedule_dynamic.c
 *
 * 运行：
 * ./schedule_dynamic
 */

#include <stdio.h>
#include <omp.h>

#define N 2000

float A[N][N], B[N][N];
float C[N][N];

/**
 * 示例 1：默认 dynamic 调度
 * 块大小 = 1，每次分配一个迭代
 */
void example1_dynamic_default() {
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

    printf("=== 示例 1：默认 dynamic 调度 ===\n");
    start_time = omp_get_wtime();
    /* schedule(dynamic)：默认块大小 = 1
     * 每次分配一个迭代给空闲线程
     * 适用于迭代计算量高度不均匀的场景 */
    #pragma omp parallel for schedule(dynamic) private(i,j,k) shared(A,B,C) num_threads(4)
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
 * 示例 2：指定块大小的 dynamic 调度
 * 每次分配指定数量的迭代
 */
void example2_dynamic_chunk() {
    int i, j, k;
    double start_time, end_time, used_time;
    float sum = 0.0;

    /* 重置矩阵 */
    for (i = 0; i < N; i++)
        for (j = 0; j < N; j++)
            C[i][j] = 0.0;

    printf("=== 示例 2：dynamic 调度，块大小 = 10 ===\n");
    start_time = omp_get_wtime();
    /* schedule(dynamic, 10)：块大小为 10
     * 每次分配 10 个迭代给空闲线程
     * 平衡调度开销和负载均衡 */
    #pragma omp parallel for schedule(dynamic,10) private(i,j,k) shared(A,B,C) num_threads(4)
    for (i = 0; i < N; i++)
        for (j = 0; j < N; j++)
            for (k = 0; k < N; k++)
                C[i][j] += A[i][k] * B[k][j];
    end_time = omp_get_wtime();
    used_time = end_time - start_time;

    for (i = 0; i < N; i++)
        for (j = 0; j < N; j++)
            sum += C[i][j];
    printf("sum=%lf, used_time=%lf seconds\n", sum, used_time);
}

int main() {
    example1_dynamic_default();
    example2_dynamic_chunk();

    return 0;
}

/**
 * dynamic 调度策略详解：
 *
 * 语法：schedule(dynamic [, chunk_size])
 *
 * 参数：
 * - chunk_size：每个块的迭代次数（可选）
 *   - 默认值：1
 *   - 块大小影响调度开销和负载均衡
 *
 * 分配方式：
 * 1. 默认块大小（chunk_size = 1）：
 *    - 每次分配一个迭代给空闲线程
 *    - 负载均衡最好
 *    - 调度开销最大
 *
 * 2. 指定块大小：
 *    - 每次分配 chunk_size 个迭代给空闲线程
 *    - 平衡调度开销和负载均衡
 *    - 块大小越大，调度开销越小，但负载均衡越差
 *
 * 优点：
 * 1. 负载均衡好（动态分配）
 * 2. 适用于迭代计算量不均匀的场景
 * 3. 可以适应动态变化的计算量
 *
 * 缺点：
 * 1. 调度开销大（运行时分配）
 * 2. 数据局部性差（可能乱序访问）
 * 3. 需要同步机制保护共享数据
 *
 * 适用场景：
 * 1. 迭代计算量不均匀
 * 2. 数据访问模式不固定
 * 3. 需要良好的负载均衡
 */
