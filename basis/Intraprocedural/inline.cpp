// 内联, 将函数调用替换为函数体, 减少函数调用开销
// 函数调用过程： 保存现场 -> 传递参数 -> 跳转函数 -> 函数体执行 -> 恢复现场 -> 返回调用点

#include <stdio.h>

#define max(a, b) ((a) > (b) ? (a) : (b))
// float max(float a, float b)
// {
//     return a > b ? a : b;
// }
int main_1()
{
    printf("%f", max(5, 6));
}

void func1(int* x, int k)
{
    x[k] = x[k] + k;
}

int main_2()
{
    int i;
    const int n = 256;
    int a[n];
    for (i = 0; i < n; i++)
    {
        a[i] = i;
    }
    for (i = 0; i < n; i++)
    {
        // func1(&a[0], i);
        a[i] = a[i] + i;
    }

    printf("%d", a[5]);
}