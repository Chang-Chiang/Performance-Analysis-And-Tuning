// 标量扩展 (Scalar Expansion)
//
// 什么是标量扩展？
//   将标量变量扩展为数组，消除循环携带依赖，使循环可以并行化。
//
// 标量扩展的作用：
//   1. 消除循环携带依赖 — 标量在迭代间传递依赖，扩展为数组后依赖消失
//   2. 便于向量化 — 每次迭代使用不同的数组元素，可以并行执行
//   3. 挖掘指令级并行 — 扩展后的操作可以乱序执行
//
// 示例 1：标量扩展消除依赖
//   优化前（有循环携带依赖）：
//     for (i = 1; i < N; i++) {
//         T = A[i];      // S1：写入 T
//         A[i] = B[i];   // S2：使用 T（依赖 S1）
//         B[i] = T;      // S3：使用 T（依赖 S1）
//     }
//
//   优化后（标量扩展）：
//     for (i = 1; i < N; i++) {
//         T[i] = A[i];      // S1：写入 T[i]
//         A[i] = B[i];      // S2
//         B[i] = T[i];      // S3：使用 T[i]
//     }
//
// 示例 2：标量重命名消除输出依赖
//   优化前（有输出依赖）：
//     T = 2;        // S1：写入 T
//     y = T + T;    // S2：读取 T
//     T = a - b;    // S3：写入 T（与 S1 有输出依赖）
//     z = T * T;    // S4：读取 T
//
//   优化后（标量重命名）：
//     T1 = 2;         // S1：写入 T1
//     y = T1 + T1;    // S2：读取 T1
//     T2 = a - b;     // S3：写入 T2
//     z = T2 * T2;    // S4：读取 T2
//
// 编译命令：
//   g++ -O2 dependency_scalar_expansion.cpp -o dependency_scalar_expansion

#include <stdio.h>

// 示例 1：标量扩展消除循环携带依赖
void example_1() {
    int A[10] = {1, 23, 4, 26, 3, 2, 6, 7, 8, 5};
    int N = sizeof(A) / sizeof(int);
    int T[10] = {0};
    int B[10] = {0};

    // 优化前：标量 T 有循环携带依赖
    // for (int i = 1; i < N; i++) {
    //     T = A[i];
    //     A[i] = B[i];
    //     B[i] = T;
    // }

    // 优化后：标量扩展为数组 T[i]
    for (int i = 1; i < N; i++) {
        T[i] = A[i];
        A[i] = B[i];
        B[i] = T[i];
    }

    printf("Example 1: A[1] = %d, B[1] = %d\n", A[1], B[1]);
}

// 示例 2：标量重命名消除输出依赖
void example_2() {
    int a = 3, b = 0;

    // 优化前：T 有输出依赖
    // int T = 2;        // S1
    // int y = T + T;    // S2
    // T = a - b;        // S3：与 S1 有输出依赖
    // int z = T * T;    // S4

    // 优化后：标量重命名
    int T1 = 2;        // S1
    int y = T1 + T1;   // S2
    int T2 = a - b;    // S3
    int z = T2 * T2;   // S4

    printf("Example 2: y = %d, z = %d\n", y, z);
}

int main() {
    example_1();
    example_2();
    return 0;
}
