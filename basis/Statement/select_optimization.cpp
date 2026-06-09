// 选择指令优化 (Select Instruction Optimization)
//
// 什么是选择指令优化？
//   将分支判断替换为选择指令（如三元运算符），消除分支跳转。
//   选择指令是无分支的条件赋值，CPU 可以流水线执行。
//
// 选择指令优化的作用：
//   1. 消除分支跳转 — 避免分支预测失败的惩罚
//   2. 提高流水线效率 — 无分支代码更容易流水线化
//   3. 减少指令数 — 选择指令比分支判断更简洁
//
// 示例分析：
//   优化前（有分支判断）：
//     if (a > 0) {
//         x = a;
//     } else {
//         x = b;
//     }
//     // 需要条件判断和跳转，可能分支预测失败
//
//   优化后（选择指令）：
//     x = (a > 0 ? a : b);
//     // 生成选择指令，无分支跳转
//
// 编译命令：
//   g++ -O2 select_optimization.cpp -o select_optimization

#include <stdio.h>

// 示例 1：优化前（分支判断）
void example_before() {
    int a = 4, b = 5;
    int x;

    // 分支判断，可能分支预测失败
    if (a > 0) {
        x = a;
    } else {
        x = b;
    }

    printf("Before: x = %d\n", x);
}

// 示例 2：优化后（选择指令）
void example_after() {
    int a = 4, b = 5;

    // 选择指令，无分支跳转
    int x = (a > 0 ? a : b);

    printf("After:  x = %d\n", x);
}

int main() {
    example_before();
    example_after();
    return 0;
}
