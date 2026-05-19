// 代数变换, 优化代数表达式, 简化计算缩短运行时间

#include<stdlib.h>
#include<stdio.h>
int main()
{
    int a = 2, b = 3;

    // a = (a + a) + (6 * a) / 2;
    // b = (b + b) + (6 * b) / 2;

    a = 5 * a;
	b = 5 * b;

    printf("a = %d, b = %d", a, b);
}