#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

#define n 1024  // 矩阵维度

int x[n][n], y[n][n], z[n][n];  // x = y * z
int a, b, i, j, k, r;

/**
 * 循环交换优化后的矩阵乘法 (循环顺序: k-i-j)
 *
 * 计算: x[i][j] = Σ y[i][k] * z[k][j]
 *
 * 优化要点:
 * 1. 将 k 循环提到最外层，i-j 循环移到内层
 * 2. 提取 y[i][k] 为局部变量 r，减少重复访问
 * 3. 内层循环 z[k][j] 和 x[i][j] 都是按行连续访问
 *
 * 性能提升原因:
 * - z[k][j]: k 固定时，j 递增 → 按行连续访问，缓存友好
 * - x[i][j]: i 固定时，j 递增 → 按行连续访问，缓存友好
 * - 缓存命中率大幅提升，内存带宽利用率提高
 */
void matrixmulti(int N, int x[n][n], int y[n][n], int z[n][n]) {
    for (k = 0; k < N; k++) {      // 最外层: 遍历共享维度
        for (i = 0; i < N; i++) {  // 中层: 遍历结果矩阵行
            r = y[i][k];           // 提取为局部变量，减少内存访问
            for (j = 0; j < N; j++) {  // 内层: 遍历结果矩阵列
                x[i][j] += r * z[k][j];  // z[k][j] 和 x[i][j] 均为连续访问
            }
        }
    }
}

int main() {
    // 初始化随机矩阵
    for (i = 0; i < n; i++) {
        for (j = 0; j < n; j++) {
            y[i][j] = rand() % 10;
            z[i][j] = rand() % 10;
            x[i][j] = 0;
        }
    }

    struct timeval starttime, endtime;
    gettimeofday(&starttime, 0);

    matrixmulti(n, x, y, z);

    gettimeofday(&endtime, 0);

    double timeuse =
        (endtime.tv_sec - starttime.tv_sec) + (endtime.tv_usec - starttime.tv_usec) / 1000000.0;

    printf("run time = %f s\n", timeuse);
    return 0;
}