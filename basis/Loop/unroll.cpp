// 循环展开
// 将循环体内的代码多次, 减少循环分支指令执行次数, 增大处理器指令调度空间
// 注意：展开次数太多, 运算过程的中间变量增加, 可能导致寄存器溢出, 反而降低性能

#include <stdio.h>
#include <x86intrin.h>
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

    // for (i = 0; i < N; i++)
    // {
    //     for (j = 0; j < N; j++)
    //     {
    //         A[i][j] = A[i][j] + B[i][j] * C[i][j];
    //     }
    // }

    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j += 4)
        {
            A[i][j] = A[i][j] + B[i][j] * C[i][j];
            A[i][j + 1] = A[i][j + 1] + B[i][j + 1] * C[i][j + 1];
            A[i][j + 2] = A[i][j + 2] + B[i][j + 2] * C[i][j + 2];
            A[i][j + 3] = A[i][j + 3] + B[i][j + 3] * C[i][j + 3];
        }

        //尾部循环
        for (j=(N / 4) * 4 + 1; j < N; j++)
        {
            A[i][j] = A[i][j] + B[i][j] * C[i][j];
        }
    }

    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
        {
            printf("%lf  ", A[i][j]);
        }
        printf("\n");
    }
}

int main()
{
    __m256 ymm0, ymm1, ymm2, ymm3, ymm4, ymm5, ymm6;
    const int N = 256;
    double A[N][N], B[N][N], C[N][N], D[N][N];
    double d[4] = { 1, 1, 1, 1 };
    double e[4] = { 2, 2, 2, 2 };
    double f[4] = { 3, 3, 3, 3 };
    int block = N / 4;
    int i, j;
    for (i = 0; i < N; i++)
    {
        for (j = 0; j < block; j++)
        {
            ymm0 = _mm256_loadu_pd(d);
            ymm1 = _mm256_loadu_pd(e);
            ymm2 = _mm256_loadu_pd(f);
            _mm256_storeu_pd(A[i] + 4 * j, ymm0);
            _mm256_storeu_pd(B[i] + 4 * j, ymm1);
            _mm256_storeu_pd(C[i] + 4 * j, ymm2);
        }
    }

    for (i = 0; i < N; i++)
    {
        for (j = 0; j < block; j++)
        {
            ymm3 = _mm256_loadu_pd(A[i] + 4 * j);
            ymm4 = _mm256_loadu_pd(B[i] + 4 * j);
            ymm5 = _mm256_loadu_pd(C[i] + 4 * j);
            ymm4 = _mm256_mul_pd(ymm4, ymm5);
            ymm6 = _mm256_add_pd(ymm3, ymm4);
            _mm256_storeu_pd(A[i] + 4 * j, ymm6);
        }
    }
    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
        {
            printf("%lf  ", A[i][j]);
        }
        printf("\n");
    }
}