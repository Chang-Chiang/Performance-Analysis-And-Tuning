/**
 * MPI 入门程序示例
 *
 * 原理：
 * 展示 MPI 编程的基本结构，包括：
 * - MPI_Init：初始化 MPI 环境
 * - MPI_Comm_rank：获取当前进程的 rank
 * - MPI_Comm_size：获取总进程数
 * - MPI_Send/MPI_Recv：进程间通信
 * - MPI_Finalize：结束 MPI 环境
 *
 * 编译指令：
 * mpicc -O2 -o mpi_hello mpi_hello.c
 *
 * 运行：
 * mpirun -np 2 ./mpi_hello
 */

#include <stdio.h>
#include <mpi.h>

int main(int argc, char *argv[])
{
    int world_rank, world_size;
    int send_data, recv_data;
    MPI_Status status;

    /* 初始化 MPI 环境
     * 必须在任何其他 MPI 函数之前调用 */
    MPI_Init(&argc, &argv);

    /* 获取当前进程的 rank（进程 ID）
     * rank 从 0 开始 */
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    /* 获取总进程数 */
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    /* 进程 0 发送数据给进程 1 */
    if (world_rank == 0)
    {
        send_data = 666;
        /* MPI_Send 参数说明：
         * &send_data: 发送缓冲区
         * 1: 发送数据的数量
         * MPI_INT: 数据类型
         * 1: 目标进程 rank
         * 0: 消息标签（tag）
         * MPI_COMM_WORLD: 通信域 */
        MPI_Send(&send_data, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
        printf("共 %d 个进程，进程 %d 成功发送数据 %d\n",
               world_size, world_rank, send_data);
    }

    /* 进程 1 接收来自进程 0 的数据 */
    if (world_rank == 1)
    {
        /* MPI_Recv 参数说明：
         * &recv_data: 接收缓冲区
         * 1: 接收数据的最大数量
         * MPI_INT: 数据类型
         * 0: 源进程 rank
         * 0: 消息标签（tag）
         * MPI_COMM_WORLD: 通信域
         * &status: 状态信息 */
        MPI_Recv(&recv_data, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
        printf("共 %d 个进程，进程 %d 成功接收数据 %d\n",
               world_size, world_rank, recv_data);
    }

    /* 结束 MPI 环境
     * 必须在所有 MPI 操作完成后调用 */
    MPI_Finalize();

    return 0;
}

/**
 * MPI 基本概念：
 *
 * 1. 进程（Process）：
 *    - MPI 程序的基本执行单位
 *    - 每个进程有独立的内存空间
 *    - 通过消息传递进行通信
 *
 * 2. 通信域（Communicator）：
 *    - 定义了一组可以互相通信的进程
 *    - MPI_COMM_WORLD 是默认的通信域，包含所有进程
 *
 * 3. Rank：
 *    - 进程在通信域中的唯一标识
 *    - 从 0 开始
 *
 * 4. 消息传递：
 *    - MPI_Send：阻塞发送
 *    - MPI_Recv：阻塞接收
 *    - 支持多种数据类型（MPI_INT, MPI_FLOAT, MPI_DOUBLE 等）
 *
 * 5. 编译和运行：
 *    - 编译：mpicc -o output source.c
 *    - 运行：mpirun -np N ./output（N 为进程数）
 */
