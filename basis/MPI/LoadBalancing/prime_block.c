/**
 * 按块分解素数筛选法
 *
 * 原理：
 * 使用按块分解（Block Decomposition）将素数筛选并行化：
 * 1. 将数轴分成连续的块
 * 2. 每个进程负责一个块
 * 3. 0 进程负责 [0, N/P)
 * 4. 进程 1 负责 [N/P, 2N/P)
 * 5. 依次类推
 *
 * 编译指令：
 * mpicc -O2 -o prime_block prime_block.c
 *
 * 运行：
 * mpirun -np 4 ./prime_block
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
    int shift, volume, left;
    double start, end;
    char *prime;
    int my_rank, world_size, index, i;

    /* 初始化 MPI */
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    /* 计算偏移量和任务量
     * 前 left 个进程多处理一个数 */
    left = MAX_PRIME % world_size;
    if (my_rank < left)
    {
        volume = MAX_PRIME / world_size + 1;
        shift = my_rank * volume;
    }
    else
    {
        volume = MAX_PRIME / world_size;
        shift = left * (volume + 1) + (my_rank - left) * volume;
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

    /* 分配存储空间 */
    prime = malloc(sizeof(char) * volume);

    /* 标记 2 的倍数 */
    for (i = shift; i < shift + volume; i++)
    {
        if (i % 2 == 0)
        {
            prime[i - shift] = 'n';
        }
        else
        {
            prime[i - shift] = 'y';
        }
    }

    /* 处理 0, 1, 2 */
    if (my_rank == 0)
    {
        prime[0] = prime[1] = 'n';
        prime[2] = 'y';
    }

    /* 初始 index 设为 3，之后由 root 进程产生 index 并广播 */
    start = MPI_Wtime();
    index = 3;

    while (index * index <= MAX_PRIME)
    {
        /* 定位到开始执行筛选的位置 */
        if (my_rank == 0)
        {
            i = index * 2;
        }
        else
        {
            i = 0;
            if (shift % index != 0)
            {
                i += index - shift % index;
            }
        }

        /* 将 index 的倍数标记为非素数 */
        for (; i + shift < volume + shift; i += index)
        {
            prime[i] = 'n';
        }

        /* 在 0 进程选择新的未标记数，之后广播到其他进程 */
        if (my_rank == 0)
        {
            for (i = index + 1; i < volume; i++)
            {
                if (prime[i] == 'y')
                {
                    index = i;
                    break;
                }
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
 * 按块分解分析：
 *
 * 1. 数据分布：
 *    - 进程 0：[0, N/P)
 *    - 进程 1：[N/P, 2N/P)
 *    - 进程 k：[kN/P, (k+1)N/P)
 *
 * 2. 负载均衡：
 *    - 每个进程处理大约 N/P 个数
 *    - 负载基本均衡
 *
 * 3. 通信模式：
 *    - 每次迭代：MPI_Bcast
 *    - 通信量：O(log P) per iteration
 *
 * 4. 缓存友好：
 *    - 每个进程访问连续的内存
 *    - 缓存命中率高
 *
 * 5. 优缺点：
 *    - 优点：缓存友好，通信量小
 *    - 缺点：负载可能不均衡（素数分布不均匀）
 */
