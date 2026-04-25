// 常数传播, 替代表达式中已知常数
// 依靠编译器, 替换代码中所有常数较为困难, 程序员尽量手动替换

#include <stdio.h>

int main()
{
    float a = 16;
    int i;
    const int n = 256;
    float x[n];
    for (i = 0; i < n; i++)
    {
        // x[i] = a / 4.0 + i;
        x[i] = 4.0 + i;
    }

    return 0;
}