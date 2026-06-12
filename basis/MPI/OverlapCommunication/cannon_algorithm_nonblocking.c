/**
 * 非阻塞式 Cannon 算法
 *
 * 原理：
 * 使用非阻塞通信（MPI_Isend/MPI_Irecv）重叠通信和计算：
 * 1. 在发送数据的同时进行计算
 * 2. 使用双缓冲区交替进行通信和计算
 * 3. 减少通信等待时间
 *
 * 编译指令：
 * mpicc -O2 -o cannon_algorithm_nonblocking cannon_algorithm_nonblocking.c
 *
 * 运行：
 * mpirun -np 4 ./cannon_algorithm_nonblocking
 */

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <mpi.h>
#include "mympi.h"

#define DIMS 1000

int main(int argc, char *argv[])
{
    int id, p, part, num, i;
    int upRank, downRank, leftRank, rightRank;
    data_t *a, *b, *c, *a1, *b1;  /* a1, b1 为非阻塞通信的缓冲区 */
    data_t *A, *B, *C;
    int coord[2], x, y;
    int position[2] = {0, 0};
    double start_time, end_time;
    MPI_Comm MPI_COMM_CART;
    MPI_Request request1, request2, request3, request4;
    MPI_Status status, status1, status2;

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

    /* 建立笛卡尔通信域 */
    size[0] = size[1] = part;
    periodic[0] = periodic[1] = 1;
    MPI_Cart_create(MPI_COMM_WORLD, 2, size, periodic, 1, &MPI_COMM_CART);
    MPI_Comm_rank(MPI_COMM_CART, &id);
    MPI_Cart_coords(MPI_COMM_CART, id, 2, coord);
    x = coord[0];
    y = coord[1];

    /* 为各个进程分配子矩阵空间
     * a, b: 当前计算使用的子矩阵
     * a1, b1: 非阻塞通信的缓冲区 */
    a = (data_t *)malloc(sizeof(data_t) * num * num);
    b = (data_t *)malloc(sizeof(data_t) * num * num);
    c = (data_t *)malloc(sizeof(data_t) * num * num);
    a1 = (data_t *)malloc(sizeof(data_t) * num * num);
    b1 = (data_t *)malloc(sizeof(data_t) * num * num);

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
    Init_Matrix(c, num * num, 1);

    /* 分发矩阵 A 和 B */
    Matrix_cannon_scatter(DIMS, id, num, part, A, a1, MPI_COMM_CART);
    Matrix_cannon_scatter(DIMS, id, num, part, B, b1, MPI_COMM_CART);

    /* 进行第一次循环位移（非阻塞）
     * A 的子块循环左移 x 位
     * B 的子块循环上移 y 位 */
    MPI_Cart_shift(MPI_COMM_CART, 1, x, &leftRank, &rightRank);
    MPI_Isend(a1, num * num, MPI_FLOAT, leftRank, i, MPI_COMM_CART, &request1);
    MPI_Irecv(a, num * num, MPI_FLOAT, rightRank, i, MPI_COMM_CART, &request2);

    MPI_Cart_shift(MPI_COMM_CART, 0, y, &upRank, &downRank);
    MPI_Isend(b1, num * num, MPI_FLOAT, upRank, i, MPI_COMM_CART, &request3);
    MPI_Irecv(b, num * num, MPI_FLOAT, downRank, i, MPI_COMM_CART, &request4);

    /* 获取上下左右相邻进程的进程号 */
    MPI_Cart_shift(MPI_COMM_CART, 0, 1, &upRank, &downRank);
    MPI_Cart_shift(MPI_COMM_CART, 1, 1, &leftRank, &rightRank);

    /* 等待第一次通信完成 */
    MPI_Wait(&request2, &status1);
    MPI_Wait(&request4, &status2);

    /* 进行余下 part 次循环位移，并计算分块 c
     * 交替使用 a, b 和 a1, b1 实现双缓冲 */
    for (i = 0; i < part; ++i)
    {
        if (i % 2 == 0)
        {
            /* 非阻塞通信：发送 a, b，接收 a1, b1 */
            MPI_Isend(a, num * num, MPI_FLOAT, leftRank, i, MPI_COMM_CART, &request1);
            MPI_Irecv(a1, num * num, MPI_FLOAT, rightRank, i, MPI_COMM_CART, &request2);
            MPI_Isend(b, num * num, MPI_FLOAT, upRank, i, MPI_COMM_CART, &request3);
            MPI_Irecv(b1, num * num, MPI_FLOAT, downRank, i, MPI_COMM_CART, &request4);

            /* 通信时计算：使用 a, b */
            Mul_Matrix(a, b, c, num, num, num);

            /* 等待通信完成 */
            MPI_Wait(&request2, &status1);
            MPI_Wait(&request4, &status2);
        }
        else
        {
            /* 非阻塞通信：发送 a1, b1，接收 a, b */
            MPI_Isend(a1, num * num, MPI_FLOAT, leftRank, i, MPI_COMM_CART, &request1);
            MPI_Irecv(a, num * num, MPI_FLOAT, rightRank, i, MPI_COMM_CART, &request2);
            MPI_Isend(b1, num * num, MPI_FLOAT, upRank, i, MPI_COMM_CART, &request3);
            MPI_Irecv(b, num * num, MPI_FLOAT, downRank, i, MPI_COMM_CART, &request4);

            /* 通信时计算：使用 a1, b1 */
            Mul_Matrix(a1, b1, c, num, num, num);

            /* 等待通信完成 */
            MPI_Wait(&request2, &status1);
            MPI_Wait(&request4, &status2);
        }
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
    free(a1);
    free(b1);

    MPI_Finalize();
    return 0;
}

/**
 * 非阻塞通信详解：
 *
 * 1. 阻塞 vs 非阻塞：
 *    - 阻塞（MPI_Send/MPI_Recv）：操作完成后才返回
 *    - 非阻塞（MPI_Isend/MPI_Irecv）：立即返回，操作在后台进行
 *
 * 2. 非阻塞通信函数：
 *    - MPI_Isend：非阻塞发送
 *    - MPI_Irecv：非阻塞接收
 *    - MPI_Wait：等待通信完成
 *    - MPI_Test：测试通信是否完成
 *
 * 3. 双缓冲技术：
 *    - 使用两组缓冲区交替进行通信和计算
 *    - a, b: 当前计算使用的缓冲区
 *    - a1, b1: 通信使用的缓冲区
 *    - 计算和通信可以同时进行
 *
 * 4. 性能优势：
 *    - 重叠通信和计算
 *    - 减少通信等待时间
 *    - 提高 GPU/CPU 利用率
 *
 * 5. 注意事项：
 *    - 需要确保通信完成后再使用数据
 *    - 缓冲区不能在通信完成前被修改
 *    - 使用 MPI_Wait 或 MPI_Test 检查通信状态
 */
