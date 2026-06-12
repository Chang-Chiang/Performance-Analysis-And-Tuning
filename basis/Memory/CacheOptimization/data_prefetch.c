/**
 * 数据预取优化示例
 *
 * 原理：
 * 数据预取（Data Prefetch）是一种通过提前将数据从内存加载到缓存中
 * 来减少缓存未命中的优化技术。
 *
 * 本例展示了使用编译器内置函数进行数据预取：
 * - __builtin_prefetch 是 GCC 提供的内置函数
 * - 可以提示 CPU 提前将数据加载到缓存中
 * - 减少缓存未命中的延迟
 *
 * 编译指令：
 * gcc -O2 -o data_prefetch data_prefetch.c
 *
 * 运行：
 * ./data_prefetch
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define N 700

int main(int argc, const char *argv[]) {
    unsigned long i, j, k;
    int res[N][N], mul1[N][N], mul2[N][N];
    clock_t start, end;
    double time1 = 0, time2 = 0;

    /* 初始化矩阵 */
    for (i = 0; i < N; ++i) {
        for (j = 0; j < N; ++j) {
            mul1[i][j] = (i + 1) * j;
            mul2[i][j] = i * j;
            res[i][j] = 0;
        }
    }

    /* 优化前：没有数据预取 */
    start = clock();
    for (i = 0; i < N; ++i) {
        for (j = 0; j < N; ++j) {
            for (k = 0; k < N; ++k) {
                res[i][j] += mul1[i][k] * mul2[k][j];
            }
        }
    }
    end = clock();
    time1 = end - start;
    printf("优化前耗时：%f 秒\n", (double)time1 / CLOCKS_PER_SEC);

    /* 重置结果矩阵 */
    for (i = 0; i < N; ++i) {
        for (j = 0; j < N; ++j) {
            res[i][j] = 0;
        }
    }

    /* 数据预取
     * __builtin_prefetch(addr, rw, locality)
     * addr: 预取的地址
     * rw: 0 表示读取，1 表示写入
     * locality: 时间局部性，0-3，0 表示没有局部性，3 表示高度局部性 */
    __builtin_prefetch(mul1, 0, 3);  /* 预取 mul1，读取，高度局部性 */
    __builtin_prefetch(mul2, 0, 0);  /* 预取 mul2，读取，无局部性 */
    __builtin_prefetch(res, 1, 3);   /* 预取 res，写入，高度局部性 */

    /* 优化后：使用数据预取 */
    start = clock();
    for (i = 0; i < N; ++i) {
        for (j = 0; j < N; ++j) {
            for (k = 0; k < N; ++k) {
                res[i][j] += mul1[i][k] * mul2[k][j];
            }
        }
    }
    end = clock();
    time2 = end - start;
    printf("优化后耗时：%f 秒\n", (double)time2 / CLOCKS_PER_SEC);

    /* 计算性能提升 */
    printf("性能提升：%.2f%%\n", (double)(time1 - time2) / time1 * 100);

    return 0;
}

/**
 * __builtin_prefetch 函数详解：
 *
 * 原型：void __builtin_prefetch(const void *addr, int rw, int locality)
 *
 * 参数：
 * - addr: 要预取的内存地址
 * - rw: 访问类型
 *   - 0: 预取用于读取（默认）
 *   - 1: 预取用于写入
 * - locality: 时间局部性
 *   - 0: 没有局部性，数据使用后立即从缓存中移除
 *   - 1: 低局部性，数据可能在缓存中保留一段时间
 *   - 2: 中等局部性
 *   - 3: 高度局部性，数据应该尽可能保留在缓存中（默认）
 *
 * 使用建议：
 * 1. 预取应该在数据实际使用前足够早的时间进行
 * 2. 预取距离应该根据目标架构调整
 * 3. 过度预取可能会污染缓存，反而降低性能
 * 4. 预取主要用于随机访问模式，顺序访问通常由硬件预取器处理
 */
