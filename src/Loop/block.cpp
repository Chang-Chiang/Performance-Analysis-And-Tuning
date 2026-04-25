// 循环分块, 对多重循环的迭代空间进行重新划分
// 流程: 先循环分段, 再交换内外层循环
// 优点: 提高程序局部性, 增加数据重用提升程序性能

#include <stdio.h>
#include <time.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))

int main()
{
    const int N = 256;

    double Total_time;
    clock_t start, end;

    float A[N][N], B[N][N], C[N][N];
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

    start = clock();
    for (j = 0; j < N; j++)
        for (k = 0; k < N; k++)
            for (i = 0; i < N; i++)
                C[i][j] = C[i][j] + A[i][k] * B[k][j];
    end = clock();
    Total_time = (double)(end - start) / CLOCKS_PER_SEC;
    printf("进行循环分块优化前：%lf秒\n", Total_time);

    // 对 i 层循环分段, 并提到最外层循环
    int I;
    int S = 4;
    start = clock();
    for (i = 0; i < N; i += S)
        for (j = 0; j < N; j++)
            for (k = 0; k < N; k++)
                for (I = i; I < MIN(i + S - 1, N); I++)
                    C[I][j] = C[I][j] + A[I][k] * B[k][j];
    end = clock();
    Total_time = (double)(end - start) / CLOCKS_PER_SEC;
    printf("进行循环分块优化一后：%lf秒\n", Total_time);

    // 对 k 层循环分段, 并提到最外层循环
    int K;
    int T = 8;
    start = clock();
    for (k = 0; k < N; k += T)
        for (i = 0; i < N; i += S)
            for (j = 0; j < N; j++)
                for (K = k; K < MIN(k + T - 1, N); K++)
                    for (I = i; I < MIN(i + S - 1, N); I++)
                        C[I][j] = C[I][j] + A[I][K] * B[K][j];
    end = clock();
    Total_time = (double)(end - start) / CLOCKS_PER_SEC;
    printf("进行循环分块优化二后：%lf秒\n", Total_time);
}