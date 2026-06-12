/**
 * 按列分解矩阵乘法
 *
 * 原理：
 * 将矩阵 A 按列分解，每个进程负责计算 C 的若干列：
 * 1. 0 进程初始化矩阵 A 和 B
 * 2. 使用自定义函数将 A 的不同列分发到各进程
 * 3. 使用 MPI_Scatter 将 B 的不同行分发到各进程
 * 4. 每个进程计算 C 的部分结果
 * 5. 使用 MPI_Reduce 归约得到最终结果
 *
 * 编译指令：
 * mpicc -O2 -o column_partition column_partition.c
 *
 * 运行：
 * mpirun -np 4 ./column_partition
 */

#include <stdio.h>
#include <mpi.h>
#include <time.h>
#include "mympi.h"

#define DIMS 1000

int main(int argc, char *argv[])
{
    data_t *A, *B, *C, *tempA, *tempB, *tempC;
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

    start_time = MPI_Wtime();

    /* 计算每个进程需要处理的列数 */
    lens = DIMS / world_size;

    /* 分配临时空间 */
    tempA = malloc(sizeof(data_t) * lens * DIMS);
    tempB = malloc(sizeof(data_t) * lens * DIMS);
    tempC = malloc(sizeof(data_t) * DIMS * DIMS);

    /* 按列分解分发矩阵 A
     * 使用自定义函数 Matrix_col_scatter */
    Matrix_col_scatter(world_rank, A, tempA, DIMS, lens, world_size, MPI_COMM_WORLD);

    /* 使用 MPI_Scatter 分发 B 的不同行 */
    MPI_Scatter(B, lens * DIMS, MPI_FLOAT,
                tempB, lens * DIMS, MPI_FLOAT,
                0, MPI_COMM_WORLD);

    /* 每个进程计算 C 的部分结果
     * tempA: DIMS x lens
     * tempB: lens x DIMS
     * tempC: DIMS x DIMS */
    Mul_Matrix(tempA, tempB, tempC, DIMS, lens, DIMS);

    /* 使用 MPI_Reduce 归约得到最终结果
     * 将所有 tempC 对应位置相加，结果存于 C */
    MPI_Reduce(tempC, C, DIMS * DIMS, MPI_FLOAT,
               MPI_SUM, 0, MPI_COMM_WORLD);

    end_time = MPI_Wtime();
    printf("进程 %d 的运行时间: %lf 秒\n", world_rank, end_time - start_time);

    /* 释放内存 */
    if (world_rank == 0)
    {
        free(A);
        free(B);
        free(C);
    }
    free(tempA);
    free(tempB);
    free(tempC);

    MPI_Finalize();
    return 0;
}

/**
 * 按列分解分析：
 *
 * 1. 数据分发：
 *    - 自定义函数将 A 的不同列分发到各进程
 *    - MPI_Scatter 将 B 的不同行分发到各进程
 *
 * 2. 并行计算：
 *    - 每个进程计算 tempC = tempA * tempB
 *    - tempA: DIMS x lens
 *    - tempB: lens x DIMS
 *    - tempC: DIMS x DIMS
 *
 * 3. 结果归约：
 *    - 使用 MPI_Reduce 将所有 tempC 相加
 *    - 0 进程获得最终结果 C
 *
 * 4. 通信开销：
 *    - 分发 A：O(N^2)
 *    - 分发 B：O(N^2 / P)
 *    - 归约 C：O(N^2)
 *
 * 5. 优缺点：
 *    - 优点：每个进程只存储部分 A 和 B
 *    - 缺点：需要归约完整的 C 矩阵
 */
