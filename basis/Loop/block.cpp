// 循环分块 (Loop Tiling / Loop Blocking)
//
// 什么是循环分块？
//   对多重循环的迭代空间进行重新划分，将大循环拆分为多个小块（tile）。
//   流程：先循环分段，再交换内外层循环。
//
// 循环分块的作用：
//   1. 提高数据局部性 — 每个小块的数据在缓存中处理，减少缓存未命中
//   2. 增加数据重用 — 同一数据在小块内被多次使用，减少内存访问
//   3. 提升性能 — 特别是矩阵乘法等计算密集型操作
//
// 优化策略：
//   1. 对 i 层循环分段 — 将 i 维度拆分为大小为 S 的小块
//   2. 对 k 层循环分段 — 将 k 维度拆分为大小为 T 的小块
//
// 编译命令：
//   g++ -O2 block.cpp -o block

#include <stdio.h>
#include <time.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))

int main() {
    const int N = 256;

    float A[N][N], B[N][N], C[N][N];

    // 初始化矩阵
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            A[i][j] = 1.0;
            B[i][j] = 2.0;
            C[i][j] = 3.0;
        }
    }

    // 优化前：标准矩阵乘法
    clock_t start = clock();
    for (int j = 0; j < N; j++) {
        for (int k = 0; k < N; k++) {
            for (int i = 0; i < N; i++) {
                C[i][j] = C[i][j] + A[i][k] * B[k][j];
            }
        }
    }
    clock_t end = clock();
    double time1 = (double)(end - start) / CLOCKS_PER_SEC;
    printf("优化前:              %.6f s\n", time1);

    // 重新初始化矩阵
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            C[i][j] = 3.0;
        }
    }

    // 优化一：对 i 层循环分段
    int S = 4;  // i 维度的分块大小
    start = clock();
    for (int i = 0; i < N; i += S) {
        for (int j = 0; j < N; j++) {
            for (int k = 0; k < N; k++) {
                for (int I = i; I < MIN(i + S, N); I++) {
                    C[I][j] = C[I][j] + A[I][k] * B[k][j];
                }
            }
        }
    }
    end = clock();
    double time2 = (double)(end - start) / CLOCKS_PER_SEC;
    printf("优化一 (i 分块 S=%d): %.6f s\n", S, time2);

    // 重新初始化矩阵
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            C[i][j] = 3.0;
        }
    }

    // 优化二：对 i 和 k 层循环都分段
    int T = 8;  // k 维度的分块大小
    start = clock();
    for (int k = 0; k < N; k += T) {
        for (int i = 0; i < N; i += S) {
            for (int j = 0; j < N; j++) {
                for (int K = k; K < MIN(k + T, N); K++) {
                    for (int I = i; I < MIN(i + S, N); I++) {
                        C[I][j] = C[I][j] + A[I][K] * B[K][j];
                    }
                }
            }
        }
    }
    end = clock();
    double time3 = (double)(end - start) / CLOCKS_PER_SEC;
    printf("优化二 (i,k 分块):   %.6f s\n", time3);

    printf("\nSpeed-up (优化一): %.2fx\n", time1 / time2);
    printf("Speed-up (优化二): %.2fx\n", time1 / time3);

    return 0;
}
