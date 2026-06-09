// 死代码删除 (Dead Code Elimination)
//
// 什么是死代码？
//   程序中不会影响程序结果的代码，包括：
//   1. 不可达代码 — return 之后的代码
//   2. 死赋值 — 变量赋值后从未被使用
//   3. 死分支 — 条件永远为 false 的分支
//
// 死代码删除的作用：
//   1. 减少代码体积 — 删除无用指令，减小程序大小
//   2. 减少执行时间 — 不再执行无意义的计算
//   3. 减少寄存器压力 — 释放被死变量占用的寄存器
//
// 示例 1：不可达代码
//   int fun() {
//       int X = 2, Y = 1, Z;
//       Z = X + 1;
//       return Z;
//       Y = 5;    // 死代码：return 之后不可达
//       return 0; // 死代码：永远不会执行
//   }
//
// 示例 2：死赋值
//   int a = 1, b = 2;
//   int c;
//   c = a + b;           // c 被赋值
//   a = (b > 0 ? a : b); // S 语句
//   c = a - b;           // c 被重新赋值，之前的 c = a + b 是死代码
//
// 编译命令：
//   g++ -O2 dce.cpp -o dce

#include <stdio.h>

// 示例 1：不可达代码
int fun_unreachable() {
    int X = 2, Y = 1, Z;
    Z = X + 1;
    return Z;
    Y = 5;     // 死代码：return 之后不可达
    return 0;  // 死代码：永远不会执行
}

// 示例 2：死赋值
void fun_dead_assignment() {
    int a = 1, b = 2;
    int c;

    c = a + b;           // c 被赋值（死赋值，因为后面被覆盖）
    a = (b > 0 ? a : b); // S 语句
    c = a - b;           // c 被重新赋值

    printf("c = %d\n", c);
}

int main() {
    printf("Example 1: Z = %d\n", fun_unreachable());
    fun_dead_assignment();
    return 0;
}
