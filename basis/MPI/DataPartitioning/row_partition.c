/**
 * 按行分解矩阵乘法
 *
 * 原理：
 * 将矩阵 A 按行分解，每个进程负责计算 C 的若干行：
 * 1. 0 进程初始化矩阵 A 和 B
 * 2. 使用 MPI_Scatter 将 A 的不同行分发到各进程
 * 3. 使用 MPI_Bcast 广播矩阵 B
 * 4. 每个进程计算 C 的对应行
 * 5. 使用 MPI_Gather 收集结果
 *
 * 编译指令：
 * mpicc -O2 -o row_partition row_partition.c
 *
 * 运行：
 * mpirun -np 4 ./row_partition
 */

#include <stdio.h>
#include <mpi.h>
#include <time.h>
#include "mympi.h"

#define DIMS 1000

int main(int argc, char *argv[])
{
    data_t *A, *B, *C, *tempA, *tempC;
    int world_rank, world_size, lens;
    double start_time, end_time;

    /* 初始化 MPI */
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    /* 检查矩阵维度是否能被进程数整除 */
    if (DIMS % world_size != 0)
    {
        if (world_rank == 0)
            printf("错误：矩阵维度 %d 必须能被进程数 %d 整除!\n", DIMS, world_size);
        MPI_Finalize();
        return 0;
    }

    /* 0 进程分配完整矩阵空间 */
    if (world_rank == 0)
    {
        A = malloc(sizeof(data_t) * DIMS * DIMS);
        B = malloc(sizeof(data_t) * DIMS * DIMS);
        C = malloc(sizeof(data_t) * DIMS * DIMS);
        Init_Matrix(A, DIMS * DIMS, 2);
        Init_Matrix(B, DIMS * DIMS, 2);
        Init_Matrix(C, DIMS * DIMS, 1);
    }

    /* 所有进程分配 B 的空间 */
    B = malloc(sizeof(data_t) * DIMS * DIMS);

    start_time = MPI_Wtime();

    /* 广播矩阵 B 到所有进程 */
    MPI_Bcast(B, DIMS * DIMS, MPI_FLOAT, 0, MPI_COMM_WORLD);

    /* 计算每个进程需要处理的行数 */
    lens = DIMS / world_size;

    /* 分配临时空间 */
    tempA = malloc(sizeof(data_t) * lens * DIMS);
    tempC = malloc(sizeof(data_t) * lens * DIMS);

    /* 使用 MPI_Scatter 分发 A 的不同行
     * 每个进程接收 lens 行数据 */
    MPI_Scatter(A, lens * DIMS, MPI_FLOAT,
                tempA, lens * DIMS, MPI_FLOAT,
                0, MPI_COMM_WORLD);

    /* 每个进程计算 C 的对应行 */
    Mul_Matrix(tempA, B, tempC, lens, DIMS, DIMS);

    /* 使用 MPI_Gather 收集结果
     * 每个进程发送 lens 行数据 */
    MPI_Gather(tempC, lens * DIMS, MPI_FLOAT,
               C, lens * DIMS, MPI_FLOAT,
               0, MPI_COMM_WORLD);

    end_time = MPI_Wtime();
    printf("进程 %d 的运行时间: %lf 秒\n", world_rank, end_time - start_time);

    /* 释放内存 */
    if (world_rank == 0)
    {
        free(A);
        free(C);
    }
    free(B);
    free(tempA);
    free(tempC);

    MPI_Finalize();
    return 0;
}

/**
 * 按行分解分析：
 *
 * 1. 数据分发：
 *    - MPI_Scatter 将 A 的不同行分发到各进程
 *    - MPI_Bcast 广播完整的 B 到所有进程
 *
 * 2. 并行计算：
 *    - 每个进程计算 C 的 lens 行
 *    - 计算量：lens * N * N = N^3 / P
 *
 * 3. 结果收集：
 *    - MPI_Gather 收集各进程的结果
 *    - 0 进程获得完整的 C 矩阵
 *
 * 4. 通信开销：
 *    - 分发 A：O(N^2 / P)
 *    - 广播 B：O(N^2)
 *    - 收集 C：O(N^2 / P)
 *
 * 5. 优缺点：
 *    - 优点：实现简单，负载均衡
 *    - 缺点：需要广播完整的 B 矩阵
 */
