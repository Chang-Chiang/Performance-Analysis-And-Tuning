// 循环倾斜 (Loop Skewing / Loop Tiling)
//
// 什么是循环倾斜？
//   通过改变循环的迭代顺序，使循环的依赖关系沿对角线方向，
//   从而可以按对角线（波前）进行并行计算。
//
// 循环倾斜的作用：
//   1. 挖掘并行性 — 将串行循环转换为可并行执行的循环
//   2. 提升数据局部性 — 沿对角线访问数据，缓存更友好
//   3. 便于向量化 — 倾斜后的循环可以使用 SIMD 指令
//
// 示例 1：二维循环倾斜
//   优化前（有依赖，无法并行）：
//     for (i = 1; i < N; i++) {
//         for (j = 1; j < N; j++) {
//             A[i][j] = A[i-1][j] + A[i][j-1];  // 依赖左方和上方
//         }
//     }
//
//   优化后（按对角线遍历，可并行）：
//     for (j = 2; j < 2 * N; j++) {
//         for (i = max(1, j-N); i < min(N, j); i++) {
//             A[i][j-i] = A[i-1][j-i] + A[i][j-i-1];
//         }
//     }
//
// 示例 2：三维循环倾斜
//   优化前：
//     for (i = 1; i < N; i++) {
//         for (j = 1; j < M; j++) {
//             for (k = 0; k < L; k++) {
//                 A[i][j][k] = A[i][j-1][k] + A[i-1][j][k];
//                 B[i][j][k+1] = B[i][j][k] + A[i][j][k];
//             }
//         }
//     }
//
//   优化后：
//     for (k = 2; k < M + L; k++) {
//         for (i = max(1, k-M-L-1); i < min(N, k+L-2); i++) {
//             for (j = max(1, k-i-L); j < min(M, k+i-1); j++) {
//                 A[i][j][k-i-j] = A[i][j-1][k-i-j] + A[i-1][j][k-i-j];
//                 B[i][j][k-i-j] = B[i][j][k-i-j] + A[i][j][k-i-j];
//             }
//         }
//     }
//
// 编译命令：
//   g++ -O2 tilting.cpp -o tilting

#include <stdio.h>

#define N         8
#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

// 示例 1：二维循环倾斜
void example_2d() {
    float A[N][N];

    // 初始化数组
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            A[i][j] = 1.0;
        }
    }

    // 优化前（有依赖，无法并行）：
    // for (int i = 1; i < N; i++) {
    //     for (int j = 1; j < N; j++) {
    //         A[i][j] = A[i-1][j] + A[i][j-1];
    //     }
    // }

    // 优化后：按对角线遍历（可并行）
    for (int j = 2; j < 2 * N; j++) {
        for (int i = max(1, j - N + 1); i < min(N, j); i++) {
            A[i][j - i] = A[i - 1][j - i] + A[i][j - i - 1];
        }
    }

    printf("2D result: A[1][1] = %f\n", A[1][1]);
}

// 示例 2：三维循环倾斜
void example_3d() {
    const int M = 8;
    const int L = 8;
    float     A[N][M][L], B[N][M][L];

    // 初始化数组
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < M; j++) {
            for (int k = 0; k < L; k++) {
                A[i][j][k] = 1.0;
                B[i][j][k] = 2.0;
            }
        }
    }

    // 优化前：
    // for (int i = 1; i < N; i++) {
    //     for (int j = 1; j < M; j++) {
    //         for (int k = 0; k < L; k++) {
    //             A[i][j][k] = A[i][j-1][k] + A[i-1][j][k];
    //             B[i][j][k+1] = B[i][j][k] + A[i][j][k];
    //         }
    //     }
    // }

    // for (i = 1; i < N ; i++) {
    //     for (j = 1; j < M ; j++) {
    //         for (k = i + j ; k < i + j + L; k++) {
    //             A[i][j][k - i - j] = A[i][j - 1][k - i - j] + A[i - 1][j][k - i - j];
    //             B[i][j][k - i - j + 1] = B[i][j][k - i - j] + A[i][j][k - i - j];
    //         }
    //     }
    // }

    // 优化后：三维循环倾斜
    for (int k = 2; k < M + L; k++) {
        for (int i = max(1, k - M - L - 1); i < min(N, k + L - 2); i++) {
            for (int j = max(1, k - i - L); j < min(M, k + i - 1); j++) {
                A[i][j][k - i - j] = A[i][j - 1][k - i - j] + A[i - 1][j][k - i - j];
                B[i][j][k - i - j] = B[i][j][k - i - j] + A[i][j][k - i - j];
            }
        }
    }

    printf("3D result: A[1][1][0] = %f\n", A[1][1][0]);
}

int main() {
    example_2d();
    example_3d();
    return 0;
}
