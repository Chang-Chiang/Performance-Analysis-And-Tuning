/**
 * 阻塞式 Cannon 算法
 *
 * 原理：
 * Cannon 算法是一种棋盘式分解的矩阵乘法算法：
 * 1. 将矩阵 A 和 B 分成 p 个子块（p 为进程数的平方根）
 * 2. 建立笛卡尔通信域，每个进程对应一个子块
 * 3. 初始对齐：A 的子块循环左移 x 位，B 的子块循环上移 y 位
 * 4. 迭代计算：每次计算后，A 左移 1 位，B 上移 1 位
 * 5. 收集结果
 *
 * 编译指令：
 * mpicc -O2 -o cannon_algorithm_blocking cannon_algorithm_blocking.c
 *
 * 运行：
 * mpirun -np 4 ./cannon_algorithm_blocking
 */

#include <stdio.h>
#include <math.h>
#include <mpi.h>
#include "mympi.h"

#define DIMS 1000

int main(int argc, char *argv[])
{
    int id, p, part, num, i;
    int upRank, downRank, leftRank, rightRank;
    data_t *a, *b, *c;
    data_t *A, *B, *C;
    int coord[2], x, y;
    int position[2] = {0, 0};
    double start_time, end_time;
    MPI_Comm MPI_COMM_CART;
    MPI_Status status;

    /* 初始化 MPI */
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &p);

    int periodic[2];
    int size[2];
    part = sqrt(p);

    /* 检查进程数是否为平方数 */
    if (part * part != p)
    {
        if (id == 0)
            printf("错误：进程数必须是一个平方数!\n");
        MPI_Finalize();
        return 0;
    }

    /* 检查矩阵维度是否能被进程数整除 */
    if (DIMS % part != 0)
    {
        if (id == 0)
            printf("错误：进程数开方之后必须能整除矩阵维度!\n");
        MPI_Finalize();
        return 0;
    }

    num = DIMS / part;

    /* 建立笛卡尔通信域
     * 2D 网格：part x part
     * 周期性边界：允许循环移位 */
    size[0] = size[1] = part;
    periodic[0] = periodic[1] = 1;
    MPI_Cart_create(MPI_COMM_WORLD, 2, size, periodic, 1, &MPI_COMM_CART);
    MPI_Comm_rank(MPI_COMM_CART, &id);
    MPI_Cart_coords(MPI_COMM_CART, id, 2, coord);
    x = coord[0];
    y = coord[1];

    /* 为各个进程分配子矩阵空间 */
    a = malloc(sizeof(data_t) * num * num);
    b = malloc(sizeof(data_t) * num * num);
    c = malloc(sizeof(data_t) * num * num);

    /* 0 进程分配完整矩阵空间并初始化 */
    if (id == 0)
    {
        A = (data_t *)malloc(sizeof(data_t) * DIMS * DIMS);
        B = (data_t *)malloc(sizeof(data_t) * DIMS * DIMS);
        C = (data_t *)malloc(sizeof(data_t) * DIMS * DIMS);
        Init_Matrix(A, DIMS * DIMS, 2);
        Init_Matrix(B, DIMS * DIMS, 2);
    }

    start_time = MPI_Wtime();

    /* 分发矩阵 A 和 B */
    Matrix_cannon_scatter(DIMS, id, num, part, A, a, MPI_COMM_CART);
    Matrix_cannon_scatter(DIMS, id, num, part, B, b, MPI_COMM_CART);
    Init_Matrix(c, num * num, 1);

    /* 进行第一次循环位移
     * A 的子块循环左移 x 位
     * B 的子块循环上移 y 位 */
    MPI_Cart_shift(MPI_COMM_CART, 1, x, &leftRank, &rightRank);
    MPI_Sendrecv_replace(a, num * num, MPI_FLOAT,
                         leftRank, 0, rightRank, 0,
                         MPI_COMM_CART, &status);

    MPI_Cart_shift(MPI_COMM_CART, 0, y, &upRank, &downRank);
    MPI_Sendrecv_replace(b, num * num, MPI_FLOAT,
                         upRank, 0, downRank, 0,
                         MPI_COMM_CART, &status);

    /* 获取上下左右相邻进程的进程号 */
    MPI_Cart_shift(MPI_COMM_CART, 0, 1, &upRank, &downRank);
    MPI_Cart_shift(MPI_COMM_CART, 1, 1, &leftRank, &rightRank);

    /* 进行余下 part 次循环位移，并计算分块 c */
    for (i = 0; i < part; ++i)
    {
        /* 计算 c += a * b */
        Mul_Matrix(a, b, c, num, num, num);

        /* A 左移 1 位，B 上移 1 位 */
        MPI_Sendrecv_replace(a, num * num, MPI_FLOAT,
                             leftRank, 0, rightRank, 0,
                             MPI_COMM_CART, &status);
        MPI_Sendrecv_replace(b, num * num, MPI_FLOAT,
                             upRank, 0, downRank, 0,
                             MPI_COMM_CART, &status);
    }

    /* 收集结果矩阵 C */
    Matrix_cannon_gather(DIMS, id, num, part, c, C, MPI_COMM_CART);

    end_time = MPI_Wtime();
    printf("进程 %d 的运行时间: %lf 秒\n", id, end_time - start_time);

    /* 释放内存 */
    MPI_Comm_free(&MPI_COMM_CART);
    if (id == 0)
    {
        free(A);
        free(B);
        free(C);
    }
    free(a);
    free(b);
    free(c);

    MPI_Finalize();
    return 0;
}

/**
 * Cannon 算法详解：
 *
 * 1. 棋盘式分解：
 *    - 将矩阵分成 sqrt(p) x sqrt(p) 个子块
 *    - 每个进程负责一个子块
 *    - 使用 2D 笛卡尔通信域
 *
 * 2. 初始对齐：
 *    - A 的子块 (i,j) 循环左移 i 位
 *    - B 的子块 (i,j) 循环上移 j 位
 *    - 确保每个进程的 A 和 B 子块可以相乘
 *
 * 3. 迭代计算：
 *    - 每次迭代：c += a * b
 *    - A 左移 1 位，B 上移 1 位
 *    - 共迭代 sqrt(p) 次
 *
 * 4. 通信模式：
 *    - 使用 MPI_Sendrecv_replace 进行循环移位
 *    - 阻塞通信，确保数据一致性
 *
 * 5. 性能分析：
 *    - 计算量：N^3 / P
 *    - 通信量：O(N^2 / sqrt(P))
 *    - 比按行/按列分解更高效
 */
