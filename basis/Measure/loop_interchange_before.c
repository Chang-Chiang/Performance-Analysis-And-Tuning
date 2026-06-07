#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

#define n 1024 // 矩阵维度

int a, b, i, j, k, r;
int x[n][n], y[n][n], z[n][n]; // x = y * z

/**
 * 标准矩阵乘法 (循环顺序: i-j-k)
 *
 * 计算: x[i][j] = Σ y[i][k] * z[k][j]
 *
 * 性能问题:
 * - 内层循环访问 z[k][j]，列索引 j 固定，k 递增
 * - C 语言按行优先存储，导致 z 矩阵的访问是跨列的（非连续）
 * - 缓存命中率低，内存访问效率差
 */
void matrixmulti(int N, int x[n][n], int y[n][n], int z[n][n]) {
    for (i = 0; i < N; i++) {     // 行遍历
        for (j = 0; j < N; j++) { // 列遍历
            r = 0;
            for (k = 0; k < N; k++) {      // 累加求和
                r = r + y[i][k] * z[k][j]; // z[k][j] 跨列访问，缓存不友好
            }
            x[i][j] = r;
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