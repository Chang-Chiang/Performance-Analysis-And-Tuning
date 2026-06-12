/**
 * 数据填充避免伪共享示例
 *
 * 原理：
 * 通过在数组元素之间添加填充（Padding），确保不同线程的数据
 * 位于不同的缓存行中，从而避免伪共享问题。
 *
 * 优化前：
 * - sum[NTHREADS] 数组，元素可能在同一缓存行中
 * - 多个线程访问不同元素时导致伪共享
 *
 * 优化后：
 * - sum[NTHREADS][8] 数组，每个元素占 8 个 double（64 字节）
 * - 确保不同线程的数据在不同缓存行中
 *
 * 编译指令：
 * gcc -O2 -fopenmp -o false_sharing_padding false_sharing_padding.c
 *
 * 运行：
 * ./false_sharing_padding
 */

#include <stdio.h>
#include <omp.h>

#define NTHREADS 4
static long num_steps = 1000000;
double step;

int main() {
    int i, j;
    double pi, start_time, used_time;

    /* 数据填充：每个线程使用独立的缓存行
     * sum[i][0]：实际使用的值
     * sum[i][1..7]：填充，确保 sum[i][0] 和 sum[i+1][0] 在不同缓存行
     *
     * 计算：8 个 double * 8 字节 = 64 字节 = 一个缓存行 */
    double sum[NTHREADS][8] = {0.0};

    step = 1.0 / (double)num_steps;

    start_time = omp_get_wtime();
    #pragma omp parallel num_threads(NTHREADS)
    {
        int i;
        int id = omp_get_thread_num();
        double x;

        /* 每个线程更新 sum[id][0]
         * 由于填充，sum[id][0] 和 sum[id+1][0] 在不同缓存行 */
        #pragma omp for
        for (i = 0; i < num_steps; i++) {
            x = (i + 0.5) * step;
            sum[id][0] += 4.0 / (1.0 + x * x);
        }
    }

    /* 汇总结果 */
    pi = 0.0;
    for (i = 0; i < NTHREADS; i++)
        pi += sum[i][0];
    pi = step * pi;

    used_time = omp_get_wtime() - start_time;
    printf("数据填充版本：pi = %15.13f, runtime = %lf seconds\n", pi, used_time);

    return 0;
}

/**
 * 数据填充原理：
 *
 * 1. 缓存行大小：64 字节
 * 2. double 类型：8 字节
 * 3. 填充计算：
 *    - 每个线程需要 1 个 double（8 字节）
 *    - 一个缓存行可以容纳 8 个 double
 *    - 使用 8 个 double 的数组，确保每个线程占用独立的缓存行
 *
 * 内存布局：
 * - sum[0][0..7]：线程 0 的数据 + 填充（64 字节）
 * - sum[1][0..7]：线程 1 的数据 + 填充（64 字节）
 * - sum[2][0..7]：线程 2 的数据 + 填充（64 字节）
 * - sum[3][0..7]：线程 3 的数据 + 填充（64 字节）
 *
 * 优点：
 * 1. 简单有效，不需要改变算法
 * 2. 确保不同线程的数据在不同缓存行
 * 3. 避免伪共享问题
 *
 * 缺点：
 * 1. 浪费内存空间（填充部分不使用）
 * 2. 需要知道缓存行大小
 * 3. 可能影响缓存利用率
 *
 * 替代方案：
 * 1. 使用 reduction 子句（更优雅）
 * 2. 使用 aligned 属性确保数据对齐
 * 3. 使用 threadprivate 存储线程私有数据
 */
