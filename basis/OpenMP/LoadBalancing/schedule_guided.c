/**
 * guided 调度策略示例
 *
 * 原理：
 * guided 调度策略是一种自适应的调度方法：
 * - 初始时分配较大的迭代块
 * - 随着迭代的进行，块大小逐渐减小
 * - 最小块大小由 chunk_size 参数指定
 *
 * 这种策略结合了 static 和 dynamic 的优点：
 * - 初始大块减少调度开销
 * - 后续小块改善负载均衡
 *
 * 语法：
 * #pragma omp for schedule(guided)      // 默认最小块大小 = 1
 * #pragma omp for schedule(guided, 10)  // 最小块大小为 10
 *
 * 编译指令：
 * gcc -O2 -fopenmp -o schedule_guided schedule_guided.c
 *
 * 运行：
 * ./schedule_guided
 */

#include <stdio.h>
#include <omp.h>

#define N 2000

float A[N][N], B[N][N];
float C[N][N];

/**
 * 示例 1：默认 guided 调度
 * 最小块大小 = 1
 */
void example1_guided_default() {
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

    printf("=== 示例 1：默认 guided 调度 ===\n");
    start_time = omp_get_wtime();
    /* schedule(guided)：默认最小块大小 = 1
     * 初始块大小 = 迭代次数 / 线程数
     * 后续块大小 = 剩余迭代 / 线程数
     * 最小块大小 = 1 */
    #pragma omp parallel for schedule(guided) private(i,j,k) shared(A,B,C) num_threads(4)
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
 * 示例 2：指定最小块大小的 guided 调度
 */
void example2_guided_chunk() {
    int i, j, k;
    double start_time, end_time, used_time;
    float sum = 0.0;

    /* 重置矩阵 */
    for (i = 0; i < N; i++)
        for (j = 0; j < N; j++)
            C[i][j] = 0.0;

    printf("=== 示例 2：guided 调度，最小块大小 = 10 ===\n");
    start_time = omp_get_wtime();
    /* schedule(guided, 10)：最小块大小为 10
     * 初始块大小 = 迭代次数 / 线程数
     * 后续块大小 = 剩余迭代 / 线程数
     * 最小块大小 = 10 */
    #pragma omp parallel for schedule(guided,10) private(i,j,k) shared(A,B,C) num_threads(4)
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
    example1_guided_default();
    example2_guided_chunk();

    return 0;
}

/**
 * guided 调度策略详解：
 *
 * 语法：schedule(guided [, chunk_size])
 *
 * 参数：
 * - chunk_size：最小块大小（可选）
 *   - 默认值：1
 *   - 块大小不会小于此值
 *
 * 分配方式：
 * 1. 初始块大小：
 *    - 块大小 = 迭代次数 / 线程数
 *    - 较大的初始块减少调度开销
 *
 * 2. 后续块大小：
 *    - 块大小 = 剩余迭代 / 线程数
 *    - 块大小逐渐减小
 *    - 最小块大小 = chunk_size
 *
 * 3. 分配顺序：
 *    - 线程 0：初始块 + 后续块
 *    - 线程 1：初始块 + 后续块
 *    - ...
 *    - 线程 N：初始块 + 后续块
 *
 * 优点：
 * 1. 初始大块减少调度开销
 * 2. 后续小块改善负载均衡
 * 3. 结合了 static 和 dynamic 的优点
 *
 * 缺点：
 * 1. 调度开销比 static 大
 * 2. 数据局部性比 static 差
 * 3. 实现复杂度较高
 *
 * 适用场景：
 * 1. 迭代计算量逐渐变化
 * 2. 需要平衡调度开销和负载均衡
 * 3. 不确定迭代计算量的分布
 *
 * 与 static 和 dynamic 的比较：
 * - static：调度开销最小，负载均衡最差
 * - dynamic：调度开销最大，负载均衡最好
 * - guided：调度开销和负载均衡介于两者之间
 */
