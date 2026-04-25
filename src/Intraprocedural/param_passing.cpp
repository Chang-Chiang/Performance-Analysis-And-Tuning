// 函数调用时, 参数通过寄存器或栈传递
// 优先通过寄存器传递, 超出寄存器值数量后通过栈传递
// 效率最该是使用寄存器, 读或写寄存器只需要 1 个时钟周期
// 而通过堆栈可能需要 数十个 时钟周期

// 向函数传递的参数越多, 开销越大 -> 将函数的参数组合成一个结构体指针

#include <stdio.h>

struct Param
{
    int x;
    int y;
    int z;
    int a;
    int b;
    int c;
};

void func(int x,int y, int z, int a, int b,int c)
{
    x = a + b;
    y = b + c;
    z = a + c;
}

void func_param(struct Param* p)
{
    p->x = p->a + p->b;
    p->y = p->b + p->c;
    p->z = p->a + p->c;
}

int main()
{
    int x, y, z, a, b, c;
    a = 1, b = 2, c = 3;

    func(x, y, z, a, b, c);
    printf("参数过多会产生额外开销\n");

    struct Param p;
    p.a = 1;
    p.b = 2;
    p.c = 3;
    func_param(&p);
    printf("传递一个指针可以大大减少函数调用时传递参数的开销\n");

    return 0;
}