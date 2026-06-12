/**
 * 循环合并示例
 *
 * 原理：
 * 循环合并（Loop Fusion）是将多个独立的循环合并为一个循环，
 * 以减少循环开销和提高数据局部性。
 *
 * 优化前：
 * - 多个独立的循环
 * - 每个循环都有循环控制开销
 * - 数据局部性差
 *
 * 优化后：
 * - 合并为一个循环
 * - 减少循环控制开销
 * - 提高数据局部性
 *
 * 编译指令：
 * gcc -O2 -fopenmp -o loop_fusion loop_fusion.c
 *
 * 运行：
 * ./loop_fusion
 */

#include <stdio.h>
#include <omp.h>

#define N 16000

int main() {
    int A[N];
    int index, id, S[5][8] = {0};
    double start, end;

    /* 初始化数组 */
    for (int i = 0; i < N; i++)
        A[i] = i % 8;

    printf("=== 循环合并示例 ===\n");
    start = omp_get_wtime();

    /* 使用 OpenMP 并行处理
     * 每个线程统计自己处理的元素
     * 最后在临界区中合并结果 */
    #pragma omp parallel private(index, id) firstprivate(A) shared(S) num_threads(4)
    {
        id = omp_get_thread_num() + 1;

        /* 每个线程统计自己的结果 */
        #pragma omp for nowait
        for (int i1 = 0; i1 < N; i1++) {
            index = A[i1];
            S[id][index]++;
        }

        /* 临界区：合并结果 */
        #pragma omp critical
        for (int j = 0; j < 8; j++)
            S[0][j] += S[id][j];
    }

    end = omp_get_wtime();
    printf("used_time = %f seconds\n", end - start);

    /* 输出统计结果 */
    printf("统计结果：\n");
    for (int i = 0; i < 8; i++)
        printf("  S[%d] = %d\n", i, S[0][i]);

    return 0;
}

/**
 * 循环合并的优点：
 *
 * 1. 减少循环开销：
 *    - 合并后只需要一个循环
 *    - 减少循环计数器更新和条件判断
 *
 * 2. 提高数据局部性：
 *    - 合并后可以同时访问多个数组
 *    - 减少缓存未命中
 *
 * 3. 提高指令级并行：
 *    - 合并后可以同时执行多个操作
 *    - 提高 CPU 流水线利用率
 *
 * 注意事项：
 * 1. 确保循环之间没有数据依赖
 * 2. 合并后的循环体不能太大
 * 3. 考虑寄存器压力
 *
 * 适用场景：
 * 1. 多个独立的循环
 * 2. 循环边界相同
 * 3. 数据访问模式相似
 */
