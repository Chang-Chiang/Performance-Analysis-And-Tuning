// 循环分裂 (Loop Splitting)
//
// 什么是循环分裂？
//   对循环的迭代次数进行拆分，将一个循环分裂为多个循环。
//   与循环分布不同，循环分裂是按迭代次数拆分，而不是按语句拆分。
//
// 循环分裂的作用：
//   1. 消除依赖 — 将有依赖的迭代和无依赖的迭代分开
//   2. 便于向量化 — 分裂后的循环可以独立优化
//   3. 减少寄存器压力 — 每个循环使用的变量更少
//
// 示例 1：按迭代位置分裂
//   优化前：
//     for (i = 1; i < N; i++) {
//         Vec[i] = Vec[i] + Vec[M];  // i == M 时有依赖
//     }
//
//   优化后：
//     for (i = 1; i < M; i++) { Vec[i] = Vec[i] + Vec[M]; }      // i < M
//     for (i = M + 1; i < N; i++) { Vec[i] = Vec[i] + Vec[M]; }  // i > M
//
// 示例 2：按语句依赖分裂（也称为循环分布）
//   优化前：
//     for (i = 0; i < N; i++) {
//         temp = a[i] - b[i];
//         coff[i] = (a[i] + b[i]) * temp;  // 依赖 temp
//         diff[i] = (c[i] + d[i]) / phi;   // 独立
//     }
//
//   优化后：
//     for (i = 0; i < N; i++) {
//         temp = a[i] - b[i];
//         coff[i] = (a[i] + b[i]) * temp;
//     }
//     for (i = 0; i < N; i++) {
//         diff[i] = (c[i] + d[i]) / phi;  // 可向量化
//     }
//
// 示例 3：按条件分裂
//   优化前：
//     for (i = 0; i < N; i++) {
//         if (i % 2 == 0) {
//             a[i] = a[i] * 2;    // 偶数索引
//         } else {
//             b[i] = b[i] * 3;    // 奇数索引
//         }
//     }
//
//   优化后：
//     for (i = 0; i < N; i += 2) { a[i] = a[i] * 2; }  // 偶数索引
//     for (i = 1; i < N; i += 2) { b[i] = b[i] * 3; }  // 奇数索引
//
// 编译命令：
//   g++ -O2 splitting.cpp -o splitting

#include <stdio.h>

#define N 100

// 示例 1：按迭代位置分裂
void example_split_by_iteration() {
    int Vec[N];
    int M = 50;

    // 初始化数组
    for (int i = 0; i < N; i++) {
        Vec[i] = i;
    }

    // 优化前：单个循环，i == M 时有依赖
    // for (int i = 1; i < N; i++) {
    //     Vec[i] = Vec[i] + Vec[M];
    // }

    // 优化后：分裂为两个循环
    for (int i = 1; i < M; i++) {
        Vec[i] = Vec[i] + Vec[M];
    }
    for (int i = M + 1; i < N; i++) {
        Vec[i] = Vec[i] + Vec[M];
    }

    printf("Example 1: Vec[1] = %d, Vec[51] = %d\n", Vec[1], Vec[51]);
}

// 示例 2：按语句依赖分裂（也称为循环分布）
void example_split_by_dependency() {
    int a[N], b[N], c[N], d[N], coff[N], diff[N];
    int phi = 2;

    // 初始化数组
    for (int i = 0; i < N; i++) {
        a[i] = i;
        b[i] = i + 1;
        c[i] = i + 2;
        d[i] = i + 3;
    }

    // 优化前：单个循环包含依赖和独立语句
    // for (int i = 0; i < N; i++) {
    //     temp = a[i] - b[i];
    //     coff[i] = (a[i] + b[i]) * temp;  // 依赖 temp
    //     diff[i] = (c[i] + d[i]) / phi;   // 独立
    // }

    // 优化后：分裂为两个循环
    for (int i = 0; i < N; i++) {
        int temp = a[i] - b[i];
        coff[i] = (a[i] + b[i]) * temp;
    }
    for (int i = 0; i < N; i++) {
        diff[i] = (c[i] + d[i]) / phi;
    }

    printf("Example 2: coff[0] = %d, diff[0] = %d\n", coff[0], diff[0]);
}

// 示例 3：按条件分裂
void example_split_by_condition() {
    int a[N], b[N];

    // 初始化数组
    for (int i = 0; i < N; i++) {
        a[i] = i;
        b[i] = i + 1;
    }

    // 优化前：循环体内有分支判断
    // for (int i = 0; i < N; i++) {
    //     if (i % 2 == 0) {
    //         a[i] = a[i] * 2;    // 偶数索引
    //     } else {
    //         b[i] = b[i] * 3;    // 奇数索引
    //     }
    // }

    // 优化后：按条件分裂为两个循环，消除分支判断
    for (int i = 0; i < N; i += 2) {
        a[i] = a[i] * 2;  // 偶数索引
    }
    for (int i = 1; i < N; i += 2) {
        b[i] = b[i] * 3;  // 奇数索引
    }

    printf("Example 3: a[0] = %d, b[1] = %d\n", a[0], b[1]);
}

int main() {
    example_split_by_iteration();
    example_split_by_dependency();
    example_split_by_condition();
    return 0;
}
