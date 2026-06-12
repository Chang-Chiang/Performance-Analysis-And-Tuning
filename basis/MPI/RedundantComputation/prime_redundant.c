/**
 * 冗余计算素数筛选法
 *
 * 原理：
 * 使用冗余计算减少通信：
 * 1. 每个进程独立生成小范围内的素数
 * 2. 使用这些素数筛选自己负责的大范围数据
 * 3. 避免了频繁的 MPI_Reduce 和 MPI_Bcast
 *
 * 编译指令：
 * mpicc -O2 -o prime_redundant prime_redundant.c
 *
 * 运行：
 * mpirun -np 4 ./prime_redundant
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
    int index, i, j, last_index, gen_prime_len;
    double start, end;
    char *prime, *gen_prime;
    int my_rank, world_size;

    /* 初始化 MPI */
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    /* 计算偏移量和任务量 */
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

    start = MPI_Wtime();

    /* 冗余计算：每个进程独立生成小范围内的素数
     * gen_prime_len 是完成 MAX_PRIME 范围的素数筛选所需要使用到的最大的未被标记数 */
    gen_prime_len = sqrt(MAX_PRIME) + 1;
    gen_prime = malloc(sizeof(char) * gen_prime_len);
    memset(gen_prime, 'y', gen_prime_len);
    gen_prime[0] = gen_prime[1] = 'n';

    last_index = -1;
    index = 2;

    /* 在 [2, gen_prime_len] 范围内筛选素数
     * 每个进程独立执行，无需通信 */
    while (index != last_index)
    {
        for (i = index * 2; i <= gen_prime_len; i += index)
        {
            gen_prime[i] = 'n';
        }
        last_index = index;
        for (i = index + 1; i <= gen_prime_len; i++)
        {
            if (gen_prime[i] == 'y')
            {
                index = i;
                break;
            }
        }
    }

    /* 使用 gen_prime 中的素数信息标记 prime 列表
     * 由于已对所有的偶数进行标记，因此直接从 3 开始标记即可 */
    for (i = 3; i < gen_prime_len; i++)
    {
        if (gen_prime[i] == 'y')
        {
            index = i;

            /* 定位到开始执行筛选的位置 */
            if (my_rank == 0)
            {
                j = index * 2;
            }
            else
            {
                j = 0;
                if (shift % index != 0)
                {
                    j += index - shift % index;
                }
            }

            /* 将 index 的倍数标记为非素数 */
            for (; j + shift < volume + shift; j += index)
            {
                prime[j] = 'n';
            }
        }
    }

    end = MPI_Wtime();
    printf("进程 %d 的运行时间: %lf 秒\n", my_rank, end - start);

    free(prime);
    free(gen_prime);
    MPI_Finalize();
    return 0;
}

/**
 * 冗余计算分析：
 *
 * 1. 冗余计算策略：
 *    - 每个进程独立生成 [2, sqrt(N)] 范围内的素数
 *    - 使用这些素数筛选自己负责的 [shift, shift+volume) 范围
 *    - 避免了频繁的 MPI_Reduce 和 MPI_Bcast
 *
 * 2. 计算量分析：
 *    - 冗余计算：O(sqrt(N) log log sqrt(N)) per process
 *    - 筛选计算：O(N/P log log N)
 *    - 总计算量：O(N/P log log N + sqrt(N) log log sqrt(N))
 *
 * 3. 通信量分析：
 *    - 通信量：0（完全无通信）
 *    - 与按块分解相比，减少了 O(log P) per iteration 的通信
 *
 * 4. 权衡：
 *    - 增加了冗余计算
 *    - 完全消除了通信
 *    - 当通信开销较大时，冗余计算更优
 *
 * 5. 适用场景：
 *    - 通信开销较大的系统
 *    - 进程数较多的情况
 *    - 计算资源充足的情况
 */
