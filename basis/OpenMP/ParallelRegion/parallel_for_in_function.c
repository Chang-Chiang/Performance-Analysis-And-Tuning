/**
 * 函数中的并行循环示例
 *
 * 原理：
 * 展示在函数中使用 OpenMP 指令的两种方式：
 * 1. 使用 #pragma omp parallel for：在函数内创建并行区
 * 2. 使用 #pragma omp for：在调用者创建的并行区内执行
 *
 * 编译指令：
 * gcc -O2 -fopenmp -o parallel_for_in_function parallel_for_in_function.c
 *
 * 运行：
 * ./parallel_for_in_function
 */

#include <stdio.h>
#include <omp.h>

#define N 2000

/**
 * 方式 1：使用 parallel for
 * 函数内创建并行区，独立运行
 */
void init_array_A(int* a) {
    #pragma omp parallel for
    for (int i = 0; i < N; i++) {
        a[i] = i;
    }
}

/**
 * 方式 2：使用 omp for
 * 必须在调用者的 parallel 区域内运行
 */
void init_array_B(int* b) {
    #pragma omp for
    for (int i = 0; i < N; i++) {
        b[i] = i;
    }
}

int main() {
    int A[N], B[N];
    double start_time, end_time, used_time;

    printf("=== 方式 1：函数内 parallel for ===\n");
    start_time = omp_get_wtime();
    init_array_A(A);  /* 独立创建并行区 */
    end_time = omp_get_wtime();
    used_time = end_time - start_time;
    printf("used_time = %lf seconds\n", used_time);

    printf("\n=== 方式 2：函数内 omp for ===\n");
    start_time = omp_get_wtime();
    #pragma omp parallel  /* 调用者创建并行区 */
    init_array_B(B);  /* 在并行区内执行 */
    end_time = omp_get_wtime();
    used_time = end_time - start_time;
    printf("used_time = %lf seconds\n", used_time);

    return 0;
}

/**
 * 两种方式的区别：
 *
 * 1. #pragma omp parallel for：
 *    - 函数内创建并行区
 *    - 独立运行，不需要外部并行区
 *    - 简单，但每次调用都会创建/销毁并行区
 *
 * 2. #pragma omp for：
 *    - 必须在 parallel 区域内使用
 *    - 复用调用者的线程池
 *    - 更高效，但需要调用者提供并行区
 *
 * 选择建议：
 * - 如果函数会被频繁调用，使用方式 2 以复用线程池
 * - 如果函数只调用一次，使用方式 1 更简单
 * - 在性能关键的代码中，尽量使用方式 2
 *
 * 示例：并行区合并
 * #pragma omp parallel
 * {
 *     init_array_B(A);  // 第一个 for 循环
 *     process_array(A); // 第二个 for 循环
 *     finalize(A);      // 第三个 for 循环
 * }
 * 这样三个函数共享同一个并行区，减少线程创建开销
 */
