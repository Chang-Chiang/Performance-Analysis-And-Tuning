/**
 * 第一个 OpenMP 程序
 *
 * 原理：
 * 展示 OpenMP 的基本使用方法，包括：
 * - #pragma omp parallel for 指令
 * - num_threads() 子句指定线程数
 * - omp_get_thread_num() 获取线程 ID
 *
 * 编译指令（需要 OpenMP 支持）：
 * gcc -O2 -fopenmp -o first_openmp_program first_openmp_program.c
 *
 * 运行：
 * ./first_openmp_program
 */

#include <stdio.h>
#include <omp.h>

#define N 16

/* 全局数组 */
int A[N], B[N], C[N];

int main() {
    /* 初始化数组 */
    for (int i = 0; i < N; i++) {
        A[i] = i;
        B[i] = 1;
        C[i] = 0;
    }

    /* OpenMP 并行化
     * #pragma omp parallel for：将下面的 for 循环并行化
     * num_threads(4)：指定使用 4 个线程
     * 每个线程处理 N/4 = 4 个数组元素 */
    #pragma omp parallel for num_threads(4)
    for (int j = 0; j < N; j++) {
        C[j] = A[j] + B[j];
        /* omp_get_thread_num()：获取当前线程的 ID
         * 可以看到不同线程处理不同的数组元素 */
        printf("thread id = %d compute A[%d]+B[%d] = %d\n",
               omp_get_thread_num(), j, j, C[j]);
    }

    return 0;
}

/**
 * OpenMP 基本概念：
 *
 * 1. 并行区（Parallel Region）：
 *    - 由 #pragma omp parallel 指令创建
 *    - 多个线程同时执行相同的代码
 *
 * 2. 工作共享（Work Sharing）：
 *    - #pragma omp for 将循环迭代分配给多个线程
 *    - 每个线程只处理部分迭代
 *
 * 3. 子句（Clause）：
 *    - num_threads(N)：指定线程数
 *    - private(var)：私有变量
 *    - shared(var)：共享变量
 *
 * 4. 运行时函数：
 *    - omp_get_thread_num()：获取线程 ID
 *    - omp_get_num_threads()：获取线程总数
 *    - omp_set_num_threads()：设置线程数
 */
