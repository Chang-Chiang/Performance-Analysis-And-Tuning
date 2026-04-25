// 两个以上的指针引用相同的存储位置时, 存在指针别名
// 导致问题: 编译器优化困难, 可能引发未定义行为
// C 中别名问题存在严重, 使用 restrict 关键字可以缓解别名问题
// C++ 中没有 restrict 关键字, 可以使用引用折叠等特性缓解别名问题

// gcc alias.c -o alias

#include <stdio.h>

#define N 1024
void add(int* a, int* b)
{
    int C = 5;
    for (int i = 0; i < N; i++)
    {
        a[i] = b[i - 1] + C;
    }
}

// restrict 限定指针变量, 表示该变量没有别名
void add_restrict(int* restrict a, int* restrict b)
{
    int C = 5;
    for (int i = 0; i < N; i++)
    {
        a[i] = b[i - 1] + C;
    }
}

int main()
{
    int a[N], b[N];
    int i;
    for (i = 0; i < N; i++)
    {
        a[i] = i;
        b[i] = i + 1;
    }
    
    add(a, b);
    add_restrict(a, b);

    printf("%d\n", a[1]);

    // add(a, a);
    // printf("%d\n", a[1]);

    // ?
    // 参数互为别名, 为错误结果
    // add_restrict(a, a);
    // printf("%d\n", a[1]);
}