/**
 * 并行区同步原语示例
 *
 * 原理：
 * 展示 OpenMP 中的各种同步原语，包括：
 * - #pragma omp single：单线程执行
 * - #pragma omp master：主线程执行
 * - #pragma omp barrier：屏障同步
 * - #pragma omp critical：临界区
 *
 * 编译指令：
 * gcc -O2 -fopenmp -o parallel_synchronization parallel_synchronization.c
 *
 * 运行：
 * ./parallel_synchronization
 */

#include <stdio.h>
#include <omp.h>

/**
 * 示例 1：single 和 master 指令
 * single：任意一个线程执行
 * master：只有主线程执行
 */
void example1_single_master() {
    int k = 2, sum1 = 0, sum2 = 0;

    printf("=== 示例 1：single 和 master 指令 ===\n");

    #pragma omp parallel firstprivate(k) shared(sum1, sum2)
    {
        /* 第一个 for 循环 */
        #pragma omp for reduction(+: sum1)
        for (int i = 0; i < 10000; i++)
            sum1 += (k + i);

        /* single 指令：任意一个线程执行
         * 其他线程等待（隐式屏障） */
        #pragma omp single
        {
            sum2 = sum1;
            printf("single: sum1 = %d\n", sum1);
        }

        /* k++ 只执行一次（由执行 single 的线程执行） */
        k++;

        /* 第二个 for 循环 */
        #pragma omp for reduction(+: sum2)
        for (int j = 0; j < 10000; j++)
            sum2 += (2 * k + j);

        /* master 指令：只有主线程（线程 0）执行
         * 其他线程不等待（无隐式屏障） */
        #pragma omp master
        printf("master: sum1 = %d, sum2 = %d\n", sum1, sum2);
    }
}

/**
 * 示例 2：nowait 子句
 * 消除 for 循环结束时的隐式屏障
 */
void example2_nowait() {
    int k = 2, sum1 = 0, sum2 = 0;

    printf("\n=== 示例 2：nowait 子句 ===\n");

    #pragma omp parallel shared(k, sum1, sum2)
    {
        /* 第一个 for 循环 */
        #pragma omp for reduction(+: sum1)
        for (int i = 0; i < 10000; i++)
            sum1 += (k + i);

        /* nowait：消除隐式屏障，线程可以继续执行 */
        #pragma omp single nowait
        {
            sum2 = sum1;
            printf("single nowait: sum1 = %d\n", sum1);
        }

        /* 注意：这里没有隐式屏障，k++ 可能在其他线程执行 for 之前或之后 */
        k++;

        /* 第二个 for 循环 */
        #pragma omp for reduction(+: sum2)
        for (int j = 0; j < 10000; j++)
            sum2 += (2 * k + j);

        /* 隐式屏障：等待所有线程完成 */
    }

    printf("结果：sum1 = %d, sum2 = %d\n", sum1, sum2);
}

int main() {
    example1_single_master();
    example2_nowait();

    return 0;
}

/**
 * 同步原语详解：
 *
 * 1. #pragma omp single：
 *    - 任意一个线程执行代码块
 *    - 其他线程等待（隐式屏障）
 *    - 可以使用 nowait 消除屏障
 *
 * 2. #pragma omp master：
 *    - 只有主线程（线程 0）执行代码块
 *    - 其他线程不等待（无隐式屏障）
 *    - 不能使用 nowait
 *
 * 3. #pragma omp barrier：
 *    - 显式屏障同步
 *    - 所有线程必须到达屏障点才能继续
 *    - 用于确保数据一致性
 *
 * 4. #pragma omp critical：
 *    - 临界区：同一时间只有一个线程执行
 *    - 用于保护共享数据的访问
 *    - 可以命名以区分不同的临界区
 *
 * 5. #pragma omp atomic：
 *    - 原子操作：保证操作的原子性
 *    - 比 critical 更高效
 *    - 只适用于简单的赋值或更新操作
 */
