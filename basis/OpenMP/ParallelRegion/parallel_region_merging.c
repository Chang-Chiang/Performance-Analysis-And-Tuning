/**
 * 并行区合并示例
 *
 * 原理：
 * 并行区合并（Parallel Region Merging）是指将多个连续的并行区
 * 合并为一个并行区，减少并行区的创建和销毁开销。
 *
 * 优化前：
 * - 两个独立的 parallel 区域
 * - 每个区域都有自己的线程池
 * - 增加了线程管理的开销
 *
 * 优化后：
 * - 合并为一个 parallel 区域
 * - 使用 firstprivate 保持变量的初始值
 * - 使用 reduction 进行归约操作
 *
 * 编译指令：
 * gcc -O2 -fopenmp -o parallel_region_merging parallel_region_merging.c
 *
 * 运行：
 * ./parallel_region_merging
 */

#include <stdio.h>
#include <omp.h>

/**
 * 优化前：两个独立的并行区
 * 每个 parallel 都会创建新的线程池
 */
void before_optimization() {
    int k = 2, sum1 = 0, sum2 = 0;

    /* 第一个并行区 */
    #pragma omp parallel shared(k)
    {
        #pragma omp for reduction(+: sum1)
        for (int i = 0; i < 10000; i++)
            sum1 += (k + i);
    }

    /* 第二个并行区 */
    #pragma omp parallel firstprivate(k)
    {
        #pragma omp for reduction(+: sum2)
        for (int j = 0; j < 10000; j++)
            sum2 += (2 * k + j);
    }

    printf("优化前：sum1 = %d, sum2 = %d\n", sum1, sum2);
}

/**
 * 优化后：合并为一个并行区
 * 使用 firstprivate 保持 k 的初始值
 */
void after_optimization() {
    int k = 2, sum1 = 0, sum2 = 0;

    /* 单个并行区 */
    #pragma omp parallel firstprivate(k)
    {
        #pragma omp for reduction(+: sum1)
        for (int i = 0; i < 10000; i++)
            sum1 += (k + i);

        #pragma omp for reduction(+: sum2)
        for (int j = 0; j < 10000; j++)
            sum2 += (k * 2 + j);
    }

    printf("优化后：sum1 = %d, sum2 = %d\n", sum1, sum2);
}

int main() {
    printf("=== 并行区合并优化 ===\n");
    before_optimization();
    after_optimization();

    return 0;
}

/**
 * 并行区合并的优点：
 *
 * 1. 减少线程创建开销：
 *    - 线程池只需创建一次
 *    - 后续的 for 循环复用已有线程
 *
 * 2. 减少同步开销：
 *    - 并行区结束时的隐式屏障减少
 *    - 线程间的同步次数减少
 *
 * 3. 提高缓存利用率：
 *    - 线程的数据局部性更好
 *    - 减少缓存失效
 *
 * 变量作用域：
 * - firstprivate(k)：每个线程有 k 的私有副本，初始值为主线程的值
 * - reduction(+:sum)：每个线程有 sum 的私有副本，最后进行加法归约
 *
 * 注意事项：
 * 1. 确保数据依赖关系正确
 * 2. 注意变量的作用域（private/shared/firstprivate/reduction）
 * 3. 使用 nowait 子句可以进一步减少同步
 */
