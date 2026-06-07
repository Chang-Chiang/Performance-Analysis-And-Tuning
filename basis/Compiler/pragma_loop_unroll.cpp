// Pragma 循环展开控制 (Pragma Loop Unrolling)
//
// 本例展示如何通过 pragma 指令控制循环展开。
//
// 常用 pragma 指令:
//   #pragma clang loop unroll(enable)      — 启用循环展开
//   #pragma clang loop unroll(disable)     — 禁用循环展开
//   #pragma clang loop unroll_count(N)     — 指定展开次数
//
// pragma 与 -O 优化的区别:
//   -O（全局策略）— 编译器自动决定展开次数
//   pragma（局部覆盖）— 针对单个循环指定展开策略
//     1. 强制展开编译器原本不会展开的循环
//     2. 禁止某个循环被展开
//     3. 指定具体的展开次数
//
// 编译命令:
//   clang -O1 pragma_unroll.cpp -o pragma_unroll
//
// 选项:
//   -O1  启用优化（pragma 需要优化支持）

#include <stdio.h>
#include <stdlib.h>

int main() {
    int N = 1024;
    int sum = 0;
    int A[N], B[N];

    for (int i = 0; i < N; ++i) {
        A[i] = i;
        B[i] = rand() % 10;
    }

    // 启用循环展开
    #pragma clang loop unroll(enable)
    for (int i = 0; i < N; i++) {
        sum = sum + A[i] + B[i];
    }

    printf("%d\n", sum);
    return 0;
}
