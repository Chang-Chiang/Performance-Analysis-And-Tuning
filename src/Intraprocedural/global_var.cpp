// 全局变量优化
// 尽量不要使用全局变量, 即便使用最好通过参数传递

#include <stdio.h>

int a = 1;

void func(int *a)
{
    int c = 14;
    *a = *a + c;
}

int main()
{
    // func();
    func(&a);
    printf("%d", a);
}