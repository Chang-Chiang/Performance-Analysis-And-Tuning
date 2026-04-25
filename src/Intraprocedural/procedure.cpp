// 过程克隆, 在不同的调用环境, 生成过程的多个实现

#include <stdio.h>
#include <stdlib.h>

#define N 8
int main()
{
    int A[20] = {0}, i, j, k;
    j = rand() % 10;
    k = 1;

    if(j = 0 || j > 4)
    {
        for (int i = 0; i < N; i++)
        {
            A[i + j] = A[i] + k;
        }
    }
    else
    {
        for (int i = 0; i < N - j; i++)
        {
            A[i + j] = A[i] + k;
        }
    }

    for (int i = 0; i < 20; i++)
    {
        printf("%d  ", A[i]);
    }
}