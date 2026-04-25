// 循环分裂, 对循环迭代次数进行拆分

#include <stdio.h>
#define N 100
int main_1()
{
    int Vec[N];
    int i, M = 50;
    for (i = 0; i < N; i++)
    {
        Vec[i] = i;
    }

    for (i = 1; i < N; i++)
    {
        Vec[i] = Vec[i] + Vec[M];
    }
    printf("由于M，该循环含有可能阻碍向量化的依赖关系\n");

    for (i = 1; i < M; i++)
    {
        Vec[i] = Vec[i] + Vec[M];
    }
    for (i = M + 1; i < N; i++)
    {
        Vec[i] = Vec[i] + Vec[M];
    }
    printf("进行循环分裂变换，得到两个循环就方便循环向量化\n");

    return 0;
}

int main()
{
    int i, temp, phi;
    int a[N], b[N], c[N], d[N], coff[N], diff[N];
    temp = 2;
    phi = 2;
    for (i = 0; i < N; i++)
    {
        a[i] = i;
        b[i] = i + 1;
        c[i] = i + 2;
        d[i] = i + 3;
    }

    // for (i = 0; i < N; i++)
    // {
    //     temp = a[i] - b[i];
    //     coff[i] = (a[i] + b[i]) * temp;
    //     diff[i] = (c[i] + d[i]) / phi;
    // }
    
    for (i = 0; i < N; i++)
    {
        temp = a[i] - b[i];
        coff[i] = (a[i] + b[i]) * temp;
    }
    for (i = 0; i < N; i++)
    {
        diff[i] = (c[i] + d[i]) / phi;
    }
}