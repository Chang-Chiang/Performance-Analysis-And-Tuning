// 循环倾斜

#include <stdio.h>

#define N 8
#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))

int main_1()
{
    float A[N][N];
    int i, j;
    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
        {
            A[i][j] = 1.0;
        }
    }

    // for (i = 1; i < N; i++)
    // {
    //     for (j = 1; j < N; j++)
    //     {
    //         A[i][j] = A[i - 1][j] + A[i][j - 1];
    //     }
    // }

    for (j = 2; j < 2 * N; j++)
    {
        for (i = max(1, j - N); i < min(N, j); i++)
        {
            A[i][j - i] = A[i - 1][j - i] + A[i][j - i - 1];
        }
    }
}

int main()
{
    const int N = 2;
    const int M = 8;
    const int L = 8;
    float A[N][M][L], B[N][M][L];
    int i, j, k;
    for (i = 0; i < N; i++)
    {
        for (j = 0; j < M; j++)
        {
            for (k = 0; k < L; k++)
            {
                A[i][j][k] = 1.0;
                B[i][j][k] = 2.0;
            }
        }
    }

    // for (i = 1; i < N; i++)
    // {
    //     for (j = 1; j < M; j++)
    //     {
    //         for (k = 0; k < L; k++)
    //         {
    //             A[i][j][k] = A[i][j - 1][k] + A[i - 1][j][k];
    //             B[i][j][k + 1] = B[i][j][k] + A[i][j][k];
    //         }
    //     }
    // }

    // for (i = 1; i < N ; i++)
    // {
    //     for (j = 1; j < M ; j++)
    //     {
    //         for (k = i + j ; k < i + j + L; k++)
    //         {
    //             A[i][j][k - i - j] = A[i][j - 1][k - i - j] + A[i - 1][j][k - i - j];
    //             B[i][j][k - i - j + 1] = B[i][j][k - i - j] + A[i][j][k - i - j];
    //         }
    //     }
    // }

    for (k = 2; k < M + L; k++)
    {
        for (i = max(1, k - M - L - 1); i < min(N, k + L - 2); i++)
        {
            for (j = max(1, k - i - L); j < min(M, k + i - 1); j++)
            {
                A[i][j][k - i - j] = A[i][j - 1][k - i - j] + A[i - 1][j][k - i - j];
                B[i][j][k - i - j] = B[i][j][k - i - j] + A[i][j][k - i - j];
            }
        }
    }

    for (i = 0; i < N; i++)
        for (j = 0; j < M; j++)
            for (k = 0; k < L; k++)
                printf("%f  ", A[i][j][k]);
}