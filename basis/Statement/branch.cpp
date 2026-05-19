// 分支语句优化

#include<stdio.h>

// 合并判断条件
int main_1()
{
    int a1 = 1, a2 = 2, a3 = 3;
    int a = 4, b = 5;

    // if ((a1 != 0) && (a2 != 0) && (a3 != 0))
    int temp = (a1 && a2 && a3);
    if (temp != 0)
    {
        a = a + b;
    }
    printf("分支判断条件是复杂表达式,会对处理器流水线的运行产生一定的影响。\n");
    return 0;
}

// 生成选择指令
int main_2()
{
    int x;
    int a = 4, b = 5;

    // 改进前
    // if (a > 0) 
    //     x = a;
    // else 
    //     x = b;

    // 改进后
    // 将分支判断移除其生成一条选择指令 
    x = (a > 0 ? a : b);

    printf( "生成选择指令，移除分支判断可以实现优化。\n");
    return 0;
}

// 条件编译
#define ON_ARM_处理器 1
#define ON_X86_处理器 2
void arm_f()
{
    printf("ON_ARM_处理器\n");
}
void x86_f()
{
    printf("ON_X86_处理器\n");
}
int main_3()
{
    int mode = ON_ARM_处理器;
    printf("条件分支代码:");

    // switch (mode)
    // {
    //     case ON_ARM_处理器:
    //         arm_f();
    //         break;
    //     case ON_X86_处理器:
    //         x86_f();
    //         break;
    // }

    #ifdef ON_ARM_处理器
        arm_f();
    #elif ON_X86_处理器
        x86_f();
    #endif

    return 0;
}

// 移除分支语句
int main()
{
    int score = 0;
    printf("请输入你的成绩：");
    scanf_s("%d", &score);

    // if (score >= 90)  //score属于（0...100）    
    //     printf("A");
    // else if (score >= 80)
    //     printf("B");
    // else if (score >= 70)
    //     printf("C");
    // else
    //     printf("D");

    char s[] = { 'D', 'D', 'D', 'D', 'D', 'D', 'D', 'C', 'B', 'A' };
    printf("%c", s[score / 10]);
}

// 平衡分支判断