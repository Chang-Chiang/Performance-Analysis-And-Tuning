// 标量重命名 (Scalar Renaming)
//
// 什么是标量重命名？
//   通过重命名变量消除输出依赖（WAW），使操作可以并行执行。
//
// 标量重命名的作用：
//   1. 消除输出依赖 — 不同的变量名消除 WAW 依赖
//   2. 增加指令级并行 — 无依赖的操作可以乱序执行
//   3. 提升流水线效率 — CPU 可以更好地调度指令
//
// 示例分析：
//   优化前（有输出依赖）：
//     int T = 2;     // S1：写入 T
//     int y = T + T; // S2：读取 T（真依赖 RAW）
//     T = a - b;     // S3：写入 T（输出依赖 WAW）
//     int z = T * T; // S4：读取 T（真依赖 RAW）
//
//     依赖关系：S1 → S2 (RAW), S1 → S3 (WAW), S3 → S4 (RAW)
//
//   优化后（标量重命名）：
//     int T1 = 2;       // S1：写入 T1
//     int y = T1 + T1;  // S2：读取 T1（真依赖 RAW）
//     int T2 = a - b;   // S3：写入 T2（无依赖）
//     int z = T2 * T2;  // S4：读取 T2（真依赖 RAW）
//
//     依赖关系：S1 → S2 (RAW), S3 → S4 (RAW)
//     S1/S2 和 S3/S4 之间无依赖，可以并行执行
//
// 编译命令：
//   g++ -O2 dependency_scalar_rename.cpp -o dependency_scalar_rename

#include <stdio.h>

// 示例 1：优化前（有输出依赖）
void example_before() {
    int a = 3, b = 0;

    int T = 2;       // S1：写入 T
    int y = T + T;   // S2：读取 T（真依赖 RAW）
    T = a - b;       // S3：写入 T（输出依赖 WAW）
    int z = T * T;   // S4：读取 T（真依赖 RAW）

    printf("Before: y = %d, z = %d\n", y, z);
}

// 示例 2：优化后（标量重命名）
void example_after() {
    int a = 3, b = 0;

    int T1 = 2;       // S1：写入 T1
    int y = T1 + T1;  // S2：读取 T1（真依赖 RAW）
    int T2 = a - b;   // S3：写入 T2（无依赖）
    int z = T2 * T2;  // S4：读取 T2（真依赖 RAW）

    printf("After:  y = %d, z = %d\n", y, z);
}

int main() {
    example_before();
    example_after();
    return 0;
}
