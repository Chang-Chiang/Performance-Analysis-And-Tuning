/**
 * 交叉分解素数筛选法
 *
 * 原理：
 * 使用交叉分解（Interleaved Decomposition）将素数筛选并行化：
 * 1. 每个进程处理不同位置的数（stride = world_size）
 * 2. 进程 0 处理 0, 4, 8, ...
 * 3. 进程 1 处理 1, 5, 9, ...
 * 4. 进程 2 处理 2, 6, 10, ...
 * 5. 进程 3 处理 3, 7, 11, ...
 *
 * 编译指令：
 * mpicc -O2 -o prime_interleaved prime_interleaved.c
 *
 * 运行：
 * mpirun -np 4 ./prime_interleaved
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <mpi.h>

#define MAX_PRIME 100000001

int main(int argc, char **argv)
{
    int volume, left;
    double start, end;
    char *prime;
    int my_rank, world_size, index, i, new_index, last_index;

    /* 初始化 MPI */
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    /* 计算每个进程的任务量
     * 前 left 个进程多处理一个数 */
    left = MAX_PRIME % world_size;
    if (my_rank < left)
    {
        volume = MAX_PRIME / world_size + 1;
    }
    else
    {
        volume = MAX_PRIME / world_size;
    }

    /* 检查进程数是否合理 */
    if (my_rank == 0)
    {
        if (volume < sqrt(MAX_PRIME))
        {
            printf("进程数太多，素数上限太小\n");
            MPI_Abort(MPI_COMM_WORLD, 0);
        }
    }

    /* 分配存储空间
     * 每个进程只存储自己负责的数 */
    prime = malloc(sizeof(char) * volume);
    memset(prime, 'y', volume);

    /* 处理 0 和 1 */
    if (my_rank == 0)
    {
        prime[0] = 'n';
    }
    if (my_rank == 1)
    {
        prime[0] = 'n';
    }

    /* 初始 index 设为 2，之后由 root 进程产生 index 并广播 */
    start = MPI_Wtime();
    index = 2;
    last_index = 0;

    while (index * index <= MAX_PRIME)
    {
        /* 筛选 index 的倍数
         * 使用交叉分解：每个进程处理不同位置的数 */
        for (i = last_index; i < volume; i++)
        {
            /* 计算全局索引：my_rank + i * world_size */
            if ((my_rank + i * world_size) % index == 0 &&
                (my_rank + i * world_size) != index)
            {
                prime[i] = 'n';
            }
            /* 标记 2 的倍数为非素数 */
            if (i != 2 && i % 2 == 0)
            {
                prime[i] = 'n';
            }
        }

        /* 寻找本进程的第一个未被标记数 */
        for (i = last_index; i < volume; i++)
        {
            if ((my_rank + i * world_size) > index && prime[i] == 'y')
            {
                index = i * world_size + my_rank;
                last_index = i;
                break;
            }
        }
        if (i == volume)
        {
            index = sqrt(MAX_PRIME) + 1;
        }

        /* 每个进程产生的 index 的最小值 */
        MPI_Reduce(&index, &new_index, 1, MPI_INT, MPI_MIN, 0, MPI_COMM_WORLD);

        /* 0 进程选择新的 index 并广播 */
        if (my_rank == 0)
        {
            if (new_index == index)
            {
                index = new_index + 1;
            }
            else
            {
                index = new_index;
            }
        }
        MPI_Bcast(&index, 1, MPI_INT, 0, MPI_COMM_WORLD);
    }

    end = MPI_Wtime();
    printf("进程 %d 的运行时间: %lf 秒\n", my_rank, end - start);

    free(prime);
    MPI_Finalize();
    return 0;
}

/**
 * 交叉分解分析：
 *
 * 1. 数据分布：
 *    - 进程 0：0, P, 2P, 3P, ...
 *    - 进程 1：1, P+1, 2P+1, 3P+1, ...
 *    - 进程 k：k, P+k, 2P+k, 3P+k, ...
 *
 * 2. 负载均衡：
 *    - 每个进程处理大约 N/P 个数
 *    - 负载基本均衡
 *
 * 3. 通信模式：
 *    - 每次迭代：MPI_Reduce + MPI_Bcast
 *    - 通信量：O(log P) per iteration
 *
 * 4. 优缺点：
 *    - 优点：负载均衡
 *    - 缺点：需要频繁通信，缓存不友好
 */
