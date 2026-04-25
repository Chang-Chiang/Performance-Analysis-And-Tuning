// 循环分布, 将一个循环体拆分为多个循环体
// 优点: 减少指令缓存压力, 增加寄存器的重用, 改善程序局部性

#include <stdio.h>

#define N 256

int main_1()
{
    int i, j;
    float C = 5, D = 6;
    float A[N], B[N];
    for (i = 0; i < N; i++)
    {
        A[i] = q;
        B[i] = 2; 
    }

    // for (i = 0; i < N; i++)
    // {
    //     A[i + 1] = A[i] + C;
    //     B[i] = B[i] + D;
    // }

    __m128 ymm0, ymm1, ymm2;
    for (i = 0; i < N; i++)
    {
        A[i + 1] = A[i] + C;
    }
    ymm0 = _mm_set_ps(D, D, D, D);
    for (i = 0; i < N; i += 4)
    {
        ymm1 = _mm_load_ps(B + i);
        ymm2 = _mm_add_ps(ymm0, ymm1);
        _mm_storeu_ps(B + i, ymm2);
    }
}

int main()
{
    int N = 256;
    float A[N][N], B[N][N], C[N][N], D;
    int i, j, k;
    D = 4.0;
    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
        {
            A[i][j] = 1.0;
            B[i][j] = 2.0;
            C[i][j] = 3.0;
        }
    }

    // for (i = 0; i < N; i++)
    // {
    //     for (j = 0; j < N; j++)
    //     {
    //         A[i][j] = D;
    //         for (k = 0; k < N; k++)
    //         {
    //             A[i][j] = A[i][j] + B[i][k] * C[k][j];
    //         }
    //     }
    // }

    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
        {
            A[i][j] = D;
        }
    }
    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
        {
            for (k = 0; k < N; k++)
            {
                A[i][j] = A[i][j] + B[i][k] * C[k][j];
            }
        }
    }
}