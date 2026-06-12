// 循环分布 (Loop Distribution / Loop Fission)
//
// 什么是循环分布？
//   将一个包含多条语句的循环拆分为多个循环，每个循环包含部分语句。
//
// 循环分布的作用：
//   1. 减少指令缓存压力 — 每个循环的指令更少，更容易放入指令缓存
//   2. 增加寄存器重用 — 每个循环使用的变量更少，寄存器压力降低
//   3. 改善程序局部性 — 每个循环只访问需要的数据，减少缓存污染
//   4. 便于其他优化 — 拆分后的循环更容易被向量化、展开等
//
// 优化前：
//   for (i = 0; i < N; i++) {
//       A[i+1] = A[i] + C;  // 有循环携带依赖
//       B[i] = B[i] + D;    // 无依赖，可向量化
//   }
//
// 优化后：
//   for (i = 0; i < N; i++) { A[i+1] = A[i] + C; }  // 有依赖，标量执行
//   for (i = 0; i < N; i++) { B[i] = B[i] + D; }    // 无依赖，可向量化
//
// 编译命令：
//   g++ -O2 -msse -Wall -o distribution distribution.cpp

#include <immintrin.h>
#include <stdio.h>

#define N 256

// 示例 1：一维数组循环分布
void example_1d() {
    float C = 5.0, D = 6.0;
    float A[N], B[N];

    // 初始化数组
    for (int i = 0; i < N; i++) {
        A[i] = 1.0;
        B[i] = 2.0;
    }

    // 优化前：单个循环包含两条语句
    // for (int i = 0; i < N; i++) {
    //     A[i + 1] = A[i] + C;  // 有循环携带依赖
    //     B[i] = B[i] + D;      // 无依赖，可向量化
    // }

    // 优化后：拆分为两个循环
    // 循环 1：有依赖，标量执行
    for (int i = 0; i < N; i++) {
        A[i + 1] = A[i] + C;
    }

    // 循环 2：无依赖，SSE 向量化
    __m128 vD = _mm_set1_ps(D);
    for (int i = 0; i < N; i += 4) {
        __m128 b = _mm_loadu_ps(&B[i]);
        b = _mm_add_ps(b, vD);
        _mm_storeu_ps(&B[i], b);
    }

    printf("1D result: A[1] = %f, B[0] = %f\n", A[1], B[0]);
}

// 示例 2：矩阵乘法循环分布
void example_2d() {
    float A[N][N], B[N][N], C[N][N];
    float D = 4.0;

    // 初始化矩阵
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            A[i][j] = 1.0;
            B[i][j] = 2.0;
            C[i][j] = 3.0;
        }
    }

    // 优化前：单个循环包含初始化和计算
    // for (int i = 0; i < N; i++) {
    //     for (int j = 0; j < N; j++) {
    //         A[i][j] = D;  // 初始化
    //         for (int k = 0; k < N; k++) {
    //             A[i][j] = A[i][j] + B[i][k] * C[k][j];  // 矩阵乘法
    //         }
    //     }
    // }

    // 优化后：拆分为两个循环
    // 循环 1：初始化
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            A[i][j] = D;
        }
    }

    // 循环 2：矩阵乘法
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            for (int k = 0; k < N; k++) {
                A[i][j] = A[i][j] + B[i][k] * C[k][j];
            }
        }
    }

    printf("2D result: A[0][0] = %f\n", A[0][0]);
}

int main() {
    example_1d();
    example_2d();
    return 0;
}
