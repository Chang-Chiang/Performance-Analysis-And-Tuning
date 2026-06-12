/**
 * 并行区扩张示例
 *
 * 原理：
 * 并行区扩张（Parallel Region Expansion）是指将多个独立的并行区
 * 合并为一个更大的并行区，减少并行区创建和销毁的开销。
 *
 * 优化前：
 * - 使用多个独立的 #pragma omp parallel for 指令
 * - 每个指令都会创建和销毁并行区
 * - 增加了线程同步和管理的开销
 *
 * 优化后：
 * - 使用一个 #pragma omp parallel 区域
 * - 在区域内使用多个 #pragma omp for 指令
 * - 减少了并行区的创建和销毁次数
 *
 * 编译指令：
 * gcc -O2 -fopenmp -o parallel_region_expansion parallel_region_expansion.c
 *
 * 运行：
 * ./parallel_region_expansion
 */

#include <stdio.h>
#include <omp.h>

#define N 2000

int A[N], B[N];

/**
 * 优化前：多个独立的并行区
 * 每个 parallel for 都会创建新的并行区
 */
void before_optimization() {
    double start_time, end_time, used_time;
    int flag = 0;

    start_time = omp_get_wtime();

    /* 第一个并行区 */
    if (flag == 0) {
        #pragma omp parallel for private(i)
        for (int i = 0; i < N; i++)
            A[i] = i;
        flag = 1;
    }

    /* 第二个并行区 */
    else {
        #pragma omp parallel for private(i)
        for (int i = 0; i < N; i++)
            B[i] = i;
    }

    end_time = omp_get_wtime();
    used_time = end_time - start_time;
    printf("优化前：used_time = %lf seconds\n", used_time);
}

/**
 * 优化后：合并为一个并行区
 * 使用 omp parallel 包裹多个 omp for
 */
void after_optimization() {
    double start_time, end_time, used_time;
    int flag = 0;

    start_time = omp_get_wtime();

    /* 单个并行区，包含多个工作共享区域 */
    #pragma omp parallel
    {
        if (flag == 1) {
            #pragma omp for
            for (int i = 0; i < N; i++)
                A[i] = i;
        } else {
            #pragma omp for
            for (int i = 0; i < N; i++)
                B[i] = i;
        }
    }

    end_time = omp_get_wtime();
    used_time = end_time - start_time;
    printf("优化后：used_time = %lf seconds\n", used_time);
}

int main() {
    printf("=== 并行区扩张优化 ===\n");
    before_optimization();
    after_optimization();

    return 0;
}

/**
 * 并行区扩张的优点：
 *
 * 1. 减少线程创建开销：
 *    - 线程池只需创建一次
 *    - 后续的 for 循环复用已有线程
 *
 * 2. 减少同步开销：
 *    - 并行区内的隐式屏障减少
 *    - 线程间的同步次数减少
 *
 * 3. 提高缓存利用率：
 *    - 线程的数据局部性更好
 *    - 减少缓存失效
 *
 * 注意事项：
 * 1. 确保数据依赖关系正确
 * 2. 注意变量的作用域（private/shared）
 * 3. 使用 nowait 子句可以进一步减少同步
 */
