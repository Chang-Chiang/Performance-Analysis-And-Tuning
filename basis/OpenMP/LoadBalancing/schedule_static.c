/**
 * static 调度策略示例
 *
 * 原理：
 * static 调度策略将循环迭代静态分配给线程：
 * - 在编译时确定每个线程处理哪些迭代
 * - 迭代块大小固定
 * - 适用于迭代计算量均匀的场景
 *
 * 语法：
 * #pragma omp for schedule(static)      // 默认块大小
 * #pragma omp for schedule(static, 10)  // 块大小为 10
 *
 * 编译指令：
 * gcc -O2 -fopenmp -o schedule_static schedule_static.c
 *
 * 运行：
 * ./schedule_static
 */

#include <stdio.h>
#include <omp.h>

#define N 2000

float A[N][N], B[N][N];
float C[N][N];

/**
 * 示例 1：默认 static 调度
 * 块大小 = 迭代次数 / 线程数
 */
void example1_static_default() {
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

    printf("=== 示例 1：默认 static 调度 ===\n");
    start_time = omp_get_wtime();
    /* schedule(static)：默认块大小
     * 块大小 = N / num_threads = 2000 / 4 = 500
     * 线程 0：迭代 0-499
     * 线程 1：迭代 500-999
     * 线程 2：迭代 1000-1499
     * 线程 3：迭代 1500-1999 */
    #pragma omp parallel for schedule(static) private(i,j,k) shared(A,B,C) num_threads(4)
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
 * 示例 2：指定块大小的 static 调度
 * 每个块包含指定数量的迭代
 */
void example2_static_chunk() {
    int i, j, k;
    double start_time, end_time, used_time;
    float sum = 0.0;

    /* 重置矩阵 */
    for (i = 0; i < N; i++)
        for (j = 0; j < N; j++)
            C[i][j] = 0.0;

    printf("=== 示例 2：static 调度，块大小 = 10 ===\n");
    start_time = omp_get_wtime();
    /* schedule(static, 10)：块大小为 10
     * 线程 0：迭代 0-9, 40-49, 80-89, ...
     * 线程 1：迭代 10-19, 50-59, 90-99, ...
     * 线程 2：迭代 20-29, 60-69, 100-109, ...
     * 线程 3：迭代 30-39, 70-79, 110-119, ... */
    #pragma omp parallel for schedule(static,10) private(i,j,k) shared(A,B,C) num_threads(4)
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
    example1_static_default();
    example2_static_chunk();

    return 0;
}

/**
 * static 调度策略详解：
 *
 * 语法：schedule(static [, chunk_size])
 *
 * 参数：
 * - chunk_size：每个块的迭代次数（可选）
 *   - 默认值：迭代次数 / 线程数
 *   - 块大小影响负载均衡和缓存利用率
 *
 * 分配方式：
 * 1. 默认块大小：
 *    - 块大小 = 迭代次数 / 线程数
 *    - 每个线程处理连续的迭代块
 *    - 适用于迭代计算量均匀的场景
 *
 * 2. 指定块大小：
 *    - 循环分配迭代块
 *    - 线程 0：块 0, 块 4, 块 8, ...
 *    - 线程 1：块 1, 块 5, 块 9, ...
 *    - 适用于迭代计算量不均匀的场景
 *
 * 优点：
 * 1. 调度开销小（编译时确定）
 * 2. 数据局部性好（连续访问）
 * 3. 适用于迭代计算量均匀的场景
 *
 * 缺点：
 * 1. 负载可能不均衡（如果迭代计算量不均匀）
 * 2. 不能适应动态变化的计算量
 *
 * 适用场景：
 * 1. 迭代计算量均匀
 * 2. 数据访问模式固定
 * 3. 需要最小化调度开销
 */
