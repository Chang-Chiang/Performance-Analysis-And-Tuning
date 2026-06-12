/**
 * 串行素数筛选法
 *
 * 原理：
 * 使用埃拉托斯特尼筛法（Sieve of Eratosthenes）查找素数：
 * 1. 假设所有数都是素数
 * 2. 从 2 开始，将所有 2 的倍数标记为非素数
 * 3. 找到下一个未标记的数，将其倍数标记为非素数
 * 4. 重复直到 sqrt(MAX_PRIME)
 *
 * 编译指令：
 * gcc -O2 -o prime_serial prime_serial.c
 *
 * 运行：
 * ./prime_serial
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define MAX_PRIME 100000001

int main()
{
    int index, i;
    double start, end;
    char *prime;

    /* 分配内存，假设所有数都是素数 */
    prime = malloc(sizeof(char) * MAX_PRIME);
    memset(prime, 'y', MAX_PRIME);

    /* 将 0 和 1 标记为非素数 */
    prime[0] = prime[1] = 'n';

    /* 标记 2 的倍数 */
    for (i = 2; i < MAX_PRIME; i++)
    {
        if (i % 2 == 0)
        {
            prime[i] = 'n';
        }
    }
    prime[2] = 'y';  /* 2 是素数 */

    start = (double)clock() / 1e6;
    index = 2;

    /* 筛选过程 */
    while (index * index <= MAX_PRIME)
    {
        /* 将 index 的倍数标记为非素数 */
        for (i = index * 2; i <= MAX_PRIME; i += index)
        {
            prime[i] = 'n';
        }

        /* 选择新的未标记数 */
        for (i = index + 1; i <= MAX_PRIME; i++)
        {
            if (prime[i] == 'y')
            {
                index = i;
                break;
            }
        }
    }

    end = (double)clock() / 1e6;
    printf("串行执行时间: %.2lf 秒\n", end - start);

    free(prime);
    return 0;
}

/**
 * 素数筛选法分析：
 *
 * 1. 算法复杂度：
 *    - 时间复杂度：O(N log log N)
 *    - 空间复杂度：O(N)
 *
 * 2. 并行化挑战：
 *    - 筛选过程有数据依赖
 *    - 需要协调多个进程的筛选范围
 *    - 需要广播新的素数
 *
 * 3. 并行化策略：
 *    - 交叉分解：每个进程处理不同位置的数
 *    - 按块分解：每个进程处理连续的数
 *    - 冗余计算：每个进程独立筛选
 */
