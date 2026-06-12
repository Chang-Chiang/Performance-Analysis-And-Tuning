// 循环压缩 (Loop Compaction / Loop Fusion)
//
// 什么是循环压缩？
//   将具有相同迭代空间的多个循环合并为一个循环，减少循环控制开销。
//   与循环合并类似，但更强调将分散的循环紧凑地合并。
//
// 循环压缩的作用：
//   1. 减少循环开销 — 合并后只需一次循环判断和跳转
//   2. 提升缓存利用率 — 同一循环内访问相同数据，减少缓存未命中
//   3. 增加指令级并行 — 合并后循环体更大，CPU 可以更好地调度指令
//
// 压缩条件：
//   1. 迭代空间相同 — 两个循环的起始、终止、步长必须一致
//   2. 无数据依赖 — 合并后不能改变程序语义
//
// 示例 1：循环分布（压缩前）
//   for (i = 1; i < N; i++) { A[i] = B[i] + C; }
//   for (i = 1; i < N; i++) { D[i] = A[i+1] + E; }
//
// 示例 2：循环压缩
//   for (i = 1; i < N; i++) {
//       A[i] = B[i] + C;
//       D[i] = A[i+1] + E;
//   }
//
// 示例 3：循环分布（压缩前）
//   for (i = 0; i < N; i++) { x[i] = a[i] + b[i]; }
//   for (i = 0; i < N; i++) { y[i] = a[i] - b[i]; }
//
// 示例 4：循环压缩
//   for (i = 0; i < N; i++) {
//       x[i] = a[i] + b[i];
//       y[i] = a[i] - b[i];
//   }
//
// 示例 5：循环分布（压缩前，有依赖）
//   for (i = 1; i < N; i++) { A[i] = B[i] + 1; }
//   for (i = 1; i < N; i++) { C[i] = A[i] + C[i-1]; }
//
// 示例 6：循环分布（压缩前，有依赖）
//   for (i = 1; i < N; i++) { A[i+1] = B[i] + C; }
//   for (i = 1; i < N; i++) { D[i] = A[i] + E; }
//
// 编译命令：
//   g++ -O2 compaction.cpp -o compaction

#include <stdio.h>

#define N 256

// 示例 1：循环分布（压缩前）
void example_1_before() {
    float A[N], B[N], C = 1.0, D[N], E = 2.0;

    for (int i = 1; i < N; i++) {
        A[i] = 1.0;
        B[i] = 2.0;
    }

    for (int i = 1; i < N; i++) {
        A[i] = B[i] + C;  // S1
    }
    for (int i = 1; i < N; i++) {
        D[i] = A[i + 1] + E;  // S2
    }

    printf("Example 1 (before): A[1] = %f, D[1] = %f\n", A[1], D[1]);
}

// 示例 2：循环压缩
void example_1_after() {
    float A[N], B[N], C = 1.0, D[N], E = 2.0;

    for (int i = 1; i < N; i++) {
        A[i] = 1.0;
        B[i] = 2.0;
    }

    for (int i = 1; i < N; i++) {
        A[i] = B[i] + C;      // S1
        D[i] = A[i + 1] + E;  // S2
    }

    printf("Example 1 (after):  A[1] = %f, D[1] = %f\n", A[1], D[1]);
}

// 示例 3：循环分布（压缩前）
void example_2_before() {
    float a[N], b[N], x[N], y[N];

    for (int i = 0; i < N; i++) {
        a[i] = 1.0;
        b[i] = 2.0;
    }

    for (int i = 0; i < N; i++) {
        x[i] = a[i] + b[i];
    }
    for (int i = 0; i < N; i++) {
        y[i] = a[i] - b[i];
    }

    printf("Example 2 (before): x[4] = %f, y[3] = %f\n", x[4], y[3]);
}

// 示例 4：循环压缩
void example_2_after() {
    float a[N], b[N], x[N], y[N];

    for (int i = 0; i < N; i++) {
        a[i] = 1.0;
        b[i] = 2.0;
    }

    for (int i = 0; i < N; i++) {
        x[i] = a[i] + b[i];
        y[i] = a[i] - b[i];
    }

    printf("Example 2 (after):  x[4] = %f, y[3] = %f\n", x[4], y[3]);
}

// 示例 5：循环分布（压缩前，有依赖）
void example_3_before() {
    float A[N], B[N], C[N];

    for (int i = 1; i < N; i++) {
        A[i] = 1.0;
        B[i] = 2.0;
        C[i] = 3.0;
    }

    for (int i = 1; i < N; i++) {
        A[i] = B[i] + 1;  // S1
    }
    for (int i = 1; i < N; i++) {
        C[i] = A[i] + C[i - 1];  // S2
    }

    printf("Example 3 (before): C[3] = %f\n", C[3]);
}

// 示例 6：循环分布（压缩前，有依赖）
void example_4_before() {
    float A[N], B[N], C = 3.0, D[N], E = 4.0;

    for (int i = 1; i < N; i++) {
        A[i] = 1.0;
        B[i] = 2.0;
    }

    for (int i = 1; i < N; i++) {
        A[i + 1] = B[i] + C;  // S1
    }
    for (int i = 1; i < N; i++) {
        D[i] = A[i] + E;  // S2
    }

    printf("Example 4 (before): D[3] = %f\n", D[3]);
}

int main() {
    example_1_before();
    example_1_after();
    example_2_before();
    example_2_after();
    example_3_before();
    example_4_before();
    return 0;
}
