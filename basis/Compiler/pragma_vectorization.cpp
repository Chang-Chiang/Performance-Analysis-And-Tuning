// Pragma 向量化提示 (Pragma Vectorization)
//
// 什么是 pragma 向量化提示？
//   通过 #pragma 指令向编译器提供向量化提示，指导编译器的向量化决策。
//   与编译选项不同，pragma 可以针对单个循环进行精细控制。
//
// pragma 与 -O 优化的区别:
//   -O（全局策略）— 对所有循环统一优化，编译器自己决定哪些向量化、用多大宽度
//   pragma（局部覆盖）— 针对单个循环指定优化策略
//     1. 强制编译器向量化它原本不会向量化的循环
//     2. 禁止某个循环被向量化
//     3. 指定向量化宽度和交错次数
//
// 应用场景:
//   1. 编译器不敢向量化 — 用 pragma 强制启用
//   2. 某个循环向量化后结果错误 — 用 pragma 禁用
//   3. 需要特定向量化宽度 — 用 pragma 指定
//
// 常用 pragma 指令:
//   #pragma clang loop vectorize(enable)    — 启用向量化
//   #pragma clang loop vectorize(disable)   — 禁用向量化
//   #pragma clang loop vectorize_width(N)   — 指定向量化宽度
//   #pragma clang loop interleave(enable)   — 启用循环交错
//   #pragma clang loop interleave_count(N)  — 指定交错次数
//
// 循环交错 (Loop Interleaving) 的作用:
//   将多次循环迭代交错执行，提升指令级并行度和流水线利用率。
//   例如：交错 2 次，每次循环执行两个向量操作
//
// 编译命令:
//   clang -O1 pragma_vectorization.cpp -o pragma_vectorization
//
// 选项:
//   -O1  启用优化（pragma 需要优化支持）

#include <stdio.h>

int main() {
    int N = 1024;
    int A[N], B[N];
    int sum = 0;

    // 启用向量化和循环交错
    #pragma clang loop vectorize(enable)
    #pragma clang loop interleave(enable)
    for (int i = 0; i < N; ++i) {
        sum = A[i] + B[i];
    }

    return sum;
}
