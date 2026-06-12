/**
 * 串行程序示例 - OpenMP 优化前的基准
 *
 * 原理：
 * 这是一个简单的串行程序，用于展示 OpenMP 并行化前的代码结构。
 * 程序对两个数组进行逐元素相加，所有计算在单线程中完成。
 *
 * 编译指令（不使用 OpenMP）：
 * gcc -O2 -o serial_example serial_example.c
 *
 * 运行：
 * ./serial_example
 */

#include <stdio.h>

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

    /* 串行计算：单线程执行 */
    for (int j = 0; j < N; j++) {
        C[j] = A[j] + B[j];
        printf("thread id = 0 compute A[%d]+B[%d] = %d\n", j, j, C[j]);
    }

    return 0;
}

/**
 * 串行程序特点：
 * 1. 所有计算在单线程中完成
 * 2. 无法利用多核处理器的并行性
 * 3. 代码简单，易于理解和调试
 *
 * OpenMP 并行化方向：
 * 1. 使用 #pragma omp parallel for 将循环并行化
 * 2. 多个线程同时处理不同的数组元素
 * 3. 可以显著提高计算密集型任务的性能
 */
