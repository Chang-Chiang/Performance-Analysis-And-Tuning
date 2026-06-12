// 过程克隆 (Procedure Cloning)
//
// 什么是过程克隆？
//   在不同的调用环境下，生成过程（函数）的多个实现。
//   编译器根据调用上下文选择最优化的版本。
//
// 过程克隆的作用：
//   1. 消除条件判断 — 针对不同条件生成专门的代码路径
//   2. 提升分支预测 — 减少运行时分支判断
//   3. 便于优化 — 每个克隆版本可以独立优化
//
// 本例分析：
//   原始代码根据 j 的值选择不同的循环边界。
//   编译器可以克隆出两个版本：
//     版本 1：j == 0 || j > 4 时执行
//     版本 2：其他情况执行
//
// 编译命令：
//   g++ -O2 procedure.cpp -o procedure

#include <stdio.h>
#include <stdlib.h>

#define N 8

int main() {
    int A[20] = {0};
    int j = rand() % 10;
    int k = 1;

    // 根据 j 的值选择不同的循环边界
    if (j == 0 || j > 4) {
        // 版本 1：j 较大或为 0
        for (int i = 0; i < N; i++) {
            A[i + j] = A[i] + k;
        }
    } else {
        // 版本 2：j 较小
        for (int i = 0; i < N - j; i++) {
            A[i + j] = A[i] + k;
        }
    }

    // 打印结果
    for (int i = 0; i < 20; i++) {
        printf("%d  ", A[i]);
    }
    printf("\n");

    return 0;
}
