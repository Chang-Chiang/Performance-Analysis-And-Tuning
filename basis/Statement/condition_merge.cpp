// 合并判断条件 (Condition Merging)
//
// 什么是合并判断条件？
//   将复杂的分支判断条件合并为一个简单的条件表达式，
//   减少分支判断次数，提高流水线效率。
//
// 合并判断条件的作用：
//   1. 简化分支判断 — 减少条件判断次数
//   2. 提高流水线效率 — 减少分支预测失败的概率
//   3. 降低指令数 — 减少比较和跳转指令
//
// 示例分析：
//   优化前（复杂条件判断）：
//     if ((a1 != 0) && (a2 != 0) && (a3 != 0)) {
//         a = a + b;
//     }
//     // 需要多次条件判断，影响流水线
//
//   优化后（合并判断条件）：
//     int temp = (a1 && a2 && a3);
//     if (temp != 0) {
//         a = a + b;
//     }
//     // 简化为一次判断，提高流水线效率
//
// 编译命令：
//   g++ -O2 condition_merge.cpp -o condition_merge

#include <stdio.h>

// 示例 1：优化前（复杂条件判断）
void example_before() {
    int a1 = 1, a2 = 2, a3 = 3;
    int a = 4, b = 5;

    // 复杂条件判断，需要多次判断
    if ((a1 != 0) && (a2 != 0) && (a3 != 0)) {
        a = a + b;
    }

    printf("Before: a = %d\n", a);
}

// 示例 2：优化后（合并判断条件）
void example_after() {
    int a1 = 1, a2 = 2, a3 = 3;
    int a = 4, b = 5;

    // 合并为一个条件，简化分支判断
    int temp = (a1 && a2 && a3);
    if (temp != 0) {
        a = a + b;
    }

    printf("After:  a = %d\n", a);
}

int main() {
    example_before();
    example_after();
    return 0;
}
