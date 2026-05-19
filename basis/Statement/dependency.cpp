// 去除相关性
// 依赖关系分为 控制依赖、数据依赖

#include <stdlib.h>
#include <stdio.h>

int main_1()
{
    int a[10] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    int x = a[0];
    int N = sizeof(a) / sizeof(int);
    for (int i = 1; i < N; i++)
    {
        if (a[i] > x) 
        {
            x = a[i];
        }
    }
    printf("x = %d", x);
}

int main_2()
{
    int A[10] = { 1, 23, 4, 26, 3, 2, 6, 7, 8, 5 };
    int N = sizeof(A) / sizeof(int);
    int B[10] = { 0 };

    // int T;
    // for (int i = 1; i < N; i++)
    // {
    //     T = A[i];
    //     A[i] = B[i];
    //     B[i] = T;
    // }

    // 标量扩展, 消除数据依赖
    int T[N];
    for (int i = 0; i < N; i++)
    {
        T[i] = A[i];
        A[i] = B[i];
        B[i] = T[i];
    }
}

int main_3()
{
    int a = 3, b = 0;
    int T = 2;
    int y = T + T;

    // T = a - b;
    // int z = T * T;

    // 标量重命名
    int T1 = a - b;
    int z = T1 * T1;
}

#define N 10
int main()
{
    int A[N] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    int B[N] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    int Y[N] = { 0 };
    int X = 1, Z = 1, C = 1;

    // for (int i = 1; i < N; i++)
    // {
    //     A[i] = A[i - 1] + X;
    //     Y[i] = A[i] + Z;
    //     A[i] = B[i] + C;
    // }

    // 数组重命名
    int A1[N] = { 0 };
    for (int i = 1; i < N; i++)
    {
        A1[i] = A[i - 1] + X;
        Y[i] = A1[i] + Z;
        A[i] = B[i] + C;
    }
}