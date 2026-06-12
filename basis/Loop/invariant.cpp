// 循环不变量外提 (Loop Invariant Code Motion)
//
// 什么是循环不变量？
//   在循环迭代过程中值不发生变化的表达式或变量。
//   例如：dt * dt、W[i] * W[i]（在内层循环中不变）
//
// 循环不变量外提的作用：
//   1. 减少重复计算 — 将不变量移到循环外，只计算一次
//   2. 减少指令数 — 循环体内指令减少，提升执行效率
//   3. 降低功耗 — 减少 CPU 计算量
//
// 注意事项：
//   外提的不变量需要占用一个寄存器。
//   如果不变量过多，可能导致寄存器溢出到栈，反而降低性能。
//
// 示例分析：
//   优化前：
//     for (i = 1; i < N; i++) {
//         for (j = 1; j < M; j++) {
//             U[i] = U[i] + W[i] * W[i] * D[j] / (dt * dt);
//         }
//     }
//
//   优化后：
//     float T1 = 1 / (dt * dt);      // 外提：除法改为乘法
//     for (i = 1; i < N; i++) {
//         float T2 = W[i] * W[i];    // 外提：移到外层循环
//         for (j = 1; j < M; j++) {
//             U[i] = U[i] + T2 * D[j] * T1;
//         }
//     }
//
// 编译命令：
//   g++ -O2 -msse2 -Wall -o invariant invariant.cpp

#include <immintrin.h>
#include <stdio.h>
#include <stdlib.h>

// 示例 1：标量代码循环不变量外提
void example_scalar() {
    const int M = 256;
    const int N = 256;
    float     U[M], W[M], D[M];
    float     dt = 5.0;

    for (int i = 1; i < N; i++) {
        U[i] = i;
        W[i] = i + 1;
        D[i] = i + 2;
    }

    // 外提不变量
    float T1 = 1 / (dt * dt); // 除法改为乘法，减少计算开销

    for (int i = 1; i < N; i++) {
        float T2 = W[i] * W[i]; // 外提：移到外层循环
        for (int j = 1; j < M; j++) {
            // 优化前：U[i] = U[i] + W[i] * W[i] * D[j] / (dt * dt);
            U[i] = U[i] + T2 * D[j] * T1;
        }
    }

    printf("U[1] = %f\n", U[1]);
}

// 示例 2：向量化代码循环不变量外提
void example_vector() {
    const int N = 16;
    float     A[N], B[N];
    float     C0 = 2.0;

    for (int i = 0; i < N; i++) {
        B[i] = 1.0;
    }

    // 外提不变量：将常量加载到向量寄存器
    __m128 v2 = _mm_set_ps1(C0);

    for (int i = 0; i < N; i += 4) {
        __m128 v1 = _mm_loadu_ps(&B[i]);
        // 优化前：v2 = _mm_set_ps1(C0);  // 每次迭代都加载
        __m128 v3 = _mm_mul_ps(v1, v2);
        _mm_store_ps(&A[i], v3);
    }

    printf("Vector result: ");
    for (int i = 0; i < N; i++) {
        printf("%.1f ", A[i]);
    }
    printf("\n");
}

int main() {
    example_scalar();
    example_vector();
    return 0;
}
