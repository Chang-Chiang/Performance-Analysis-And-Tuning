/**
 * collapse 指令示例 - 循环嵌套合并调度
 *
 * 原理：
 * collapse(N) 子句将 N 层嵌套循环合并为一个大的迭代空间，
 * 然后将合并后的迭代分配给多个线程。
 *
 * 优化前：
 * - 只有最外层循环被并行化
 * - 如果外层循环迭代次数少，线程利用率低
 *
 * 优化后：
 * - 使用 collapse(2) 将两层循环合并
 * - 增加总的迭代次数，提高线程利用率
 * - 改善负载均衡
 *
 * 编译指令：
 * gcc -O2 -fopenmp -o collapse_directive collapse_directive.c
 *
 * 运行：
 * ./collapse_directive
 */

#include <stdio.h>
#include <omp.h>
#include <unistd.h>

/**
 * 示例 1：collapse 基本用法
 * 将两层循环合并为一个迭代空间
 */
void example1_collapse_basic() {
    int i, j;
    double start_time, end_time, used_time;

    printf("=== 示例 1：collapse 基本用法 ===\n");

    start_time = omp_get_wtime();
    /* collapse(2)：将两层循环合并
     * 合并后的迭代空间：4 * 8 = 32 个迭代
     * 8 个线程，每个线程处理 4 个迭代 */
    #pragma omp parallel for private(i,j) collapse(2) num_threads(8)
    for (i = 0; i < 4; i++)
        for (j = 0; j < 8; j++)
            usleep(10000);  /* 模拟计算 */
    end_time = omp_get_wtime();
    used_time = end_time - start_time;

    printf("used_time = %lf seconds\n\n", used_time);
}

/**
 * 示例 2：collapse 在矩阵乘法中的应用
 * 将三层循环的前两层合并
 */
void example2_collapse_matrix() {
    #define N 2000
    float A[N][N], B[N][N], C[N][N];
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

    printf("=== 示例 2：collapse 在矩阵乘法中的应用 ===\n");

    start_time = omp_get_wtime();
    /* collapse(2)：将 i 和 j 循环合并
     * 合并后的迭代空间：N * N = 4000000 个迭代
     * 4 个线程，每个线程处理 1000000 个迭代 */
    #pragma omp parallel for collapse(2) private(i,j,k) shared(A,B,C) num_threads(4)
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

    printf("sum=%lf, used_time=%lf seconds\n", sum, used_time);
}

int main() {
    example1_collapse_basic();
    example2_collapse_matrix();

    return 0;
}

/**
 * collapse 子句详解：
 *
 * 语法：#pragma omp for collapse(N)
 *
 * 参数 N：
 * - 指定要合并的循环层数
 * - N 必须是编译时常量
 * - 合并从外到内的 N 层循环
 *
 * 工作原理：
 * 1. 将 N 层嵌套循环展开为一维迭代空间
 * 2. 计算总的迭代次数：product(iteration_count[i])
 * 3. 将一维迭代分配给多个线程
 * 4. 每个线程处理分配到的迭代
 *
 * 优点：
 * 1. 增加总的迭代次数，提高线程利用率
 * 2. 改善负载均衡，特别是当外层循环迭代次数少时
 * 3. 简化调度，编译器可以更好地优化
 *
 * 注意事项：
 * 1. 循环迭代之间不能有数据依赖
 * 2. 循环边界必须是编译时常量或私有变量
 * 3. 合并后的迭代空间不能太大，否则调度开销增加
 *
 * 与 schedule 子句的配合：
 * - collapse 可以与 schedule 子句一起使用
 * - schedule 控制合并后迭代的分配方式
 */
