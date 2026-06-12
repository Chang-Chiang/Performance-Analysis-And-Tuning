/**
 * 基础并行矩阵乘法
 *
 * 原理：
 * 使用 MPI 将矩阵乘法并行化：
 * 1. 0 进程初始化矩阵 A 和 B
 * 2. 广播矩阵 A 和 B 到所有进程
 * 3. 每个进程计算 C 的若干行
 * 4. 收集结果到 0 进程
 *
 * 编译指令：
 * mpicc -O2 -o matrix_multiply_basic_parallel matrix_multiply_basic_parallel.c
 *
 * 运行：
 * mpirun -np 4 ./matrix_multiply_basic_parallel
 */

#include <stdio.h>
#include <mpi.h>
#include "mympi.h"

#define DIMS 1000

int main(int argc, char *argv[])
{
    data_t *A, *B, *C;
    int world_rank, world_size, lens, i;
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

    /* 为所有进程分配内存 */
    A = malloc(sizeof(data_t) * DIMS * DIMS);
    B = malloc(sizeof(data_t) * DIMS * DIMS);
    C = malloc(sizeof(data_t) * DIMS * DIMS);
    Init_Matrix(C, DIMS * DIMS, 1);

    /* 0 进程初始化矩阵 A 和 B */
    if (world_rank == 0)
    {
        Init_Matrix(A, DIMS * DIMS, 2);
        Init_Matrix(B, DIMS * DIMS, 2);
    }

    start_time = MPI_Wtime();

    /* 广播矩阵 A 和 B 到所有进程
     * MPI_Bcast 参数说明：
     * 数据缓冲区, 数据数量, 数据类型, 根进程, 通信域 */
    MPI_Bcast(A, DIMS * DIMS, MPI_FLOAT, 0, MPI_COMM_WORLD);
    MPI_Bcast(B, DIMS * DIMS, MPI_FLOAT, 0, MPI_COMM_WORLD);

    /* 计算每个进程需要处理的行数 */
    lens = DIMS / world_size;

    /* 每个进程计算 C 的对应行
     * A + lens * DIMS * world_rank: 偏移到当前进程对应的 A 的行
     * C + lens * DIMS * world_rank: 偏移到当前进程对应的 C 的行 */
    Mul_Matrix(A + lens * DIMS * world_rank, B,
               C + lens * DIMS * world_rank, lens, DIMS, DIMS);

    /* 各进程将自身计算的 C 广播到其他进程 */
    for (i = 0; i < world_size; i++)
    {
        MPI_Bcast(C + i * lens * DIMS, lens * DIMS, MPI_FLOAT, i, MPI_COMM_WORLD);
    }

    end_time = MPI_Wtime();
    printf("进程 %d 的运行时间: %lf 秒\n", world_rank, end_time - start_time);

    /* 释放内存 */
    free(A);
    free(B);
    free(C);

    MPI_Finalize();
    return 0;
}

/**
 * 基础并行矩阵乘法分析：
 *
 * 1. 数据分发：
 *    - 0 进程初始化矩阵 A 和 B
 *    - 使用 MPI_Bcast 广播到所有进程
 *
 * 2. 并行计算：
 *    - 每个进程计算 C 的若干行
 *    - 行数 = DIMS / world_size
 *
 * 3. 结果收集：
 *    - 每个进程广播自己计算的行
 *    - 所有进程获得完整的 C 矩阵
 *
 * 4. 通信开销：
 *    - 广播 A 和 B：O(N^2)
 *    - 收集 C：O(N^2)
 *    - 计算：O(N^3 / P)
 *
 * 5. 优化方向：
 *    - 使用 MPI_Scatter/MPI_Gather 代替 MPI_Bcast
 *    - 减少通信次数
 *    - 重叠通信和计算
 */
