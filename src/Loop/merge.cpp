// 循环合并, 将具有相同迭代空间的循环合成一个循环, 减少循环控制开销

#include <stdio.h>

#define N 256 

int main()
{
    int i;
    float a[N], b[N], x[N], y[N];
    for (i = 0; i < N; i++)
    {
        a[i] = 1.0;
        b[i] = 2.0;
    }

    // for (i = 0; i < N; i++)
    // {
    //     x[i] = a[i] + b[i];
    // }
    // for (i = 0; i < N; i++)
    // {
    //     y[i] = a[i] - b[i];
    // }

    // 循环合并
    for (i = 0; i < N; i++)
    {
        x[i] = a[i] + b[i];
        y[i] = a[i] - b[i];
    }

    printf("%f\n", x[4]);
    printf("%f\n", y[3]);
}