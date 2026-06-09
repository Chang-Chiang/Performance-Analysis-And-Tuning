// 分支优化 (Branch Optimization)
//
// 什么是分支优化？
//   通过算法重构消除分支判断，使用查表等技术替代条件分支。
//
// 分支优化的作用：
//   1. 消除分支预测失败 — 无分支代码避免预测失败的惩罚
//   2. 提高流水线效率 — 无分支代码更容易流水线化
//   3. 减少指令数 — 查表比多次判断更简洁
//
// 示例分析：
//   优化前（多分支判断）：
//     if (score >= 90) printf("A");
//     else if (score >= 80) printf("B");
//     else if (score >= 70) printf("C");
//     else printf("D");
//     // 多次分支判断，可能分支预测失败
//
//   优化后（查表法）：
//     char s[] = {'D', 'D', 'D', 'D', 'D', 'D', 'D', 'C', 'B', 'A'};
//     printf("%c", s[score / 10]);
//     // 无分支，直接查表
//
// 编译命令：
//   g++ -O2 branch_optimization.cpp -o branch_optimization

#include <stdio.h>

// 示例 1：优化前（多分支判断）
void example_before() {
    int score = 85;

    printf("Before: ");
    if (score >= 90) {
        printf("A");
    } else if (score >= 80) {
        printf("B");
    } else if (score >= 70) {
        printf("C");
    } else {
        printf("D");
    }
    printf("\n");
}

// 示例 2：优化后（查表法）
void example_after() {
    int score = 85;

    // 查表法：用数组索引替代分支判断
    char s[] = {'D', 'D', 'D', 'D', 'D', 'D', 'D', 'C', 'B', 'A'};
    printf("After:  %c\n", s[score / 10]);
}

int main() {
    example_before();
    example_after();
    return 0;
}
