// 删除冗余语句, 如未运行的代码、声明了但未用到的变量等
// 减少程序大小, 避免程序在运行中进行不相关的运算行为, 减少运行的时间

#include<stdio.h>
int fun(void)
{
    int X = 2, Y = 1, Z;
    Z = X + 1;
    Y = 5;//死代码
    return Z;
}

int main_1()
{
    fun();
}

int main() {
    int a = 1, b = 2;
    int c, d;

    // 冗余语句
    // if (b > 0)
    // {
    //     c = a + b;
    // }
    // else
    // {
    //     c = a - b;
    // }

    // d = c + 1;

    d = a + 2;
    
    printf("%d\n", d);

    return 0;
}