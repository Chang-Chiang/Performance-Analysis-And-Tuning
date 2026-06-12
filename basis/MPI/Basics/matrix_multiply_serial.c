/**
 * 串行矩阵乘法
 *
 * 原理：
 * 传统的三重循环矩阵乘法，用于 MPI 并行化的基准对比。
 *
 * 编译指令：
 * gcc -O2 -o matrix_multiply_serial matrix_multiply_serial.c
 *
 * 运行：
 * ./matrix_multiply_serial
 */

#include <stdio.h>
#include <time.h>
#include "mympi.h"

#define DIMS 1000

int main(int argc, char *argv[])
{
    data_t *A, *B, *C;
    double start_time, end_time;

    /* 分配内存 */
    A = (data_t *)malloc(sizeof(data_t) * DIMS * DIMS);
    B = (data_t *)malloc(sizeof(data_t) * DIMS * DIMS);
    C = (data_t *)malloc(sizeof(data_t) * DIMS * DIMS);

    /* 初始化矩阵
     * 参数 2 表示随机生成 0/1 矩阵
     * 参数 1 表示生成 0 矩阵 */
    Init_Matrix(A, DIMS * DIMS, 2);
    Init_Matrix(B, DIMS * DIMS, 2);
    Init_Matrix(C, DIMS * DIMS, 1);

    /* 串行矩阵乘法 */
    start_time = (double)clock();
    Mul_Matrix(A, B, C, DIMS, DIMS, DIMS);
    end_time = (double)clock();

    printf("串行执行时间: %.2lf ms\n", (end_time - start_time) / 1e3);

    /* 释放内存 */
    free(A);
    free(B);
    free(C);

    return 0;
}

/**
 * 串行矩阵乘法分析：
 *
 * 1. 时间复杂度：O(N^3)
 * 2. 空间复杂度：O(N^2)
 * 3. 对于 N=1000，计算量约为 10 亿次浮点运算
 *
 * 并行化方向：
 * 1. 按行分解：每个进程计算 C 的若干行
 * 2. 按列分解：每个进程计算 C 的若干列
 * 3. 棋盘式分解：每个进程计算 C 的一个子块
 */
