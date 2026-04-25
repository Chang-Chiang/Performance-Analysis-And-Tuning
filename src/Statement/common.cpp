// 公共子表达式, 仅计算一次

#include<stdio.h>
#include<stdlib.h>
int main()
{
    int a = 1, b = 5;
    int tmp = a + b;
    
    // if ((a + b) > 3 && (a + b) < 10)
    if (tmp > 3 && tmp < 10)
    {
        a = a + b;
    }
    printf("%d", a);
}