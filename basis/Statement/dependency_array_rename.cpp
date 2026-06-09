// 数组重命名 (Array Renaming)
//
// 什么是数组重命名？
//   通过重命名数组消除输出依赖（WAW），使操作可以并行执行。
//
// 数组重命名的作用：
//   1. 消除输出依赖 — 不同的数组名消除 WAW 依赖
//   2. 增加指令级并行 — 无依赖的操作可以乱序执行
//   3. 提升流水线效率 — CPU 可以更好地调度指令
//
// 示例分析：
//   优化前（有输出依赖）：
//     for (i = 1; i < N; i++) {
//         A[i] = A[i-1] + X;  // S1：写入 A[i]
//         Y[i] = A[i] + Z;    // S2：读取 A[i]（真依赖 RAW）
//         A[i] = B[i] + C;    // S3：写入 A[i]（输出依赖 WAW）
//     }
//
//     依赖关系：S1 → S2 (RAW), S1 → S3 (WAW), S2 → S3 (RAW)
//
//   优化后（数组重命名）：
//     for (i = 1; i < N; i++) {
//         A1[i] = A[i-1] + X;  // S1：写入 A1[i]
//         Y[i] = A1[i] + Z;    // S2：读取 A1[i]（真依赖 RAW）
//         A[i] = B[i] + C;     // S3：写入 A[i]（无依赖）
//     }
//
//     依赖关系：S1 → S2 (RAW)
//     S1/S2 和 S3 之间无依赖，可以并行执行
//
// 编译命令：
//   g++ -O2 dependency_array_rename.cpp -o dependency_array_rename

#include <stdio.h>

#define N 10

// 示例 1：优化前（有输出依赖）
void example_before() {
    int A[N] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    int B[N] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    int Y[N] = {0};
    int X = 1, Z = 1, C = 1;

    for (int i = 1; i < N; i++) {
        A[i] = A[i - 1] + X;  // S1：写入 A[i]
        Y[i] = A[i] + Z;      // S2：读取 A[i]（真依赖 RAW）
        A[i] = B[i] + C;      // S3：写入 A[i]（输出依赖 WAW）
    }

    printf("Before: A[1] = %d, Y[1] = %d\n", A[1], Y[1]);
}

// 示例 2：优化后（数组重命名）
void example_after() {
    int A[N] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    int B[N] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    int Y[N] = {0};
    int A1[N] = {0};
    int X = 1, Z = 1, C = 1;

    for (int i = 1; i < N; i++) {
        A1[i] = A[i - 1] + X;  // S1：写入 A1[i]
        Y[i] = A1[i] + Z;      // S2：读取 A1[i]（真依赖 RAW）
        A[i] = B[i] + C;       // S3：写入 A[i]（无依赖）
    }

    printf("After:  A[1] = %d, Y[1] = %d\n", A[1], Y[1]);
}

int main() {
    example_before();
    example_after();
    return 0;
}
