// VLIW 矩阵乘法 (VLIW Matrix Multiplication)
//
// 什么是 VLIW？
//   VLIW（Very Long Instruction Word，超长指令字）是一种指令级并行技术，
//   将多条指令打包成一条超长指令，由多个功能单元同时执行。
//
// VLIW 的特点：
//   1. 指令级并行 — 多条指令同时执行
//   2. 编译器调度 — 编译器负责指令调度和并行化
//   3. 硬件简单 — 不需要复杂的乱序执行逻辑
//
// 本例实现矩阵乘法 C = A * B：
//   for (i = 0; i < N; i++) {
//       for (j = 0; j < N; j++) {
//           c[i][j] = 0;
//           for (k = 0; k < N; k++) {
//               c[i][j] += a[i][k] * b[k][j];
//           }
//       }
//   }
//
// 编译命令：
//   g++ -O2 vliw.cpp -o vliw

#include <stdio.h>

#define N 10

float A[N][N], B[N][N], C[N][N];

int main() {
    // 初始化矩阵 A
    printf("Matrix A:\n");
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            A[i][j] = i + j;
            printf("%6.1f ", A[i][j]);
        }
        printf("\n");
    }

    // 初始化矩阵 B
    printf("\nMatrix B:\n");
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            B[i][j] = i - j;
            printf("%6.1f ", B[i][j]);
        }
        printf("\n");
    }

    // 矩阵乘法 C = A * B
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            C[i][j] = 0;
            for (int k = 0; k < N; k++) {
                C[i][j] += A[i][k] * B[k][j];
            }
        }
    }

    // 打印结果矩阵 C
    printf("\nMatrix C = A * B:\n");
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            printf("%6.1f ", C[i][j]);
        }
        printf("\n");
    }

    return 0;
}
