// 循环交换, 交换循环中的嵌套顺序, 增强数据局部性

#include <stdio.h>
#include <time.h>
#include <immintrin.h>

int main_1()
{
    const int N = 256;
    double A[N][N], B[N][N], C[N][N];
    int i, j, k;
    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
        {
            A[i][j] = 1.0;
            B[i][j] = 2.0;
            C[i][j] = 3.0;
        }
    }

    // for (j = 0; j < N; j++)
    // {
    //     for (k = 0; k < N; k++)
    //     {
    //         for (i = 0; i < N; i++)
    //         {
    //             A[i][j] = A[i][j] + B[i][k] * C[k][j];
    //         }
    //     }
    // }

    // 交换内外层循环
    // for (j = 0; j < N; j++)
    // {
    //     for (i = 0; i < N; i++)
    //     {
    //         for (k = 0; k < N; k++)
    //         {
    //             A[i][j] = A[i][j] + B[i][k] * C[k][j];
    //         }
    //     }
    // }

    __m128d VA, VB, VC;
    for (j = 0; j < N; j++)
    {
        for (i = 0; i < N; i++)
        {
            VA = _mm_loadu_pd(&A[i][j]);
            for (k = 0; k < N; k++)
            {
                VC = _mm_loadu_pd(&C[k][j]);
                VB = _mm_set1_pd(B[i][k]);
                VB = _mm_mul_pd(VB, VC);
                VA = _mm_add_pd(VA, VB);
            }
            _mm_storeu_pd(&A[i][j], VA);
        }
    }

    return 0;
}

int main()
{
    const int M = 1000, N = 1000;
    double Total_time;
    clock_t start, end;
    int i, j;
    float A[N][N];
    for (i = 0; i < M; i++)
    {
        for (j = 0; j < N; j++)
        {
            A[i][j] = j;
        }
    }

    start = clock();
    for (i = 1; i < M; i++)
    {
        for (j = 1; j < N; j++)
        {
            A[i][j] = A[i - 1][j];
        }
    }
    end = clock();
    Total_time = (double)(end - start) / CLOCKS_PER_SEC;
    printf("进行循环交换优化前：%lf秒\n", Total_time);

    start = clock();
    for (j = 1; j < N; j++)
    {
        for (i = 1; i < M; i++)
        {
            A[i][j] = A[i - 1][j];
        }
    }
    end = clock();
    Total_time = (double)(end - start) / CLOCKS_PER_SEC;
    printf("进行循环交换优化后：%lf秒\n", Total_time);
}