/**
 * 结构体拆分优化示例
 *
 * 原理：
 * 结构体拆分（Struct Splitting）通过将结构体拆分为多个小结构体，
 * 使每个小结构体只包含相关数据，提高缓存命中率。
 *
 * 优化前：
 * - 使用一个大结构体包含所有数据
 * - 访问部分数据时，需要加载整个结构体
 * - 浪费缓存空间
 *
 * 优化后：
 * - 将结构体拆分为多个小结构体
 * - 每个小结构体只包含相关数据
 * - 访问时只加载需要的数据
 *
 * 编译指令：
 * gcc -O2 -o struct_splitting struct_splitting.c
 *
 * 运行：
 * ./struct_splitting
 */

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <malloc.h>

/**
 * 优化前的结构体
 * 包含所有维度的数据
 */
typedef struct {
    float t_x, t_y, t_z;  /* 时间分量 */
    float v_x, v_y, v_z;  /* 速度分量 */
    float d_x, d_y, d_z;  /* 位移分量 */
} motion_full;

/**
 * 优化后的结构体 - 只包含 x 维度的数据
 * 如果只需要访问 x 维度，使用这个结构体可以减少内存占用
 */
typedef struct {
    float t_x, v_x, d_x;  /* x 维度：时间、速度、位移 */
} motion_x;

/**
 * 优化后的结构体 - 包含 y 和 z 维度的数据
 */
typedef struct {
    float t_y, v_y, d_y;  /* y 维度：时间、速度、位移 */
    float t_z, v_z, d_z;  /* z 维度：时间、速度、位移 */
} motion_yz;

int main() {
    int N = 10000000;
    int i;
    clock_t start, end;
    double Total_time;

    /* 分配内存 */
    motion_full* P_full = (motion_full*)malloc(N * sizeof(motion_full));
    motion_x* P_x = (motion_x*)malloc(N * sizeof(motion_x));

    /* 初始化数据 */
    for (i = 0; i < N; i++) {
        P_full[i].t_x = rand() % 10;
        P_full[i].v_x = rand() % 10;
        P_x[i].t_x = rand() % 10;
        P_x[i].v_x = rand() % 10;
    }

    /* 优化前：使用完整结构体 */
    start = clock();
    for (i = 1; i <= N; i++) {
        P_full[i].d_x = P_full[i].t_x * P_full[i].v_x;
    }
    end = clock();
    Total_time = (double)(end - start) / CLOCKS_PER_SEC;
    printf("结构拆分优化前：%lf秒\n", Total_time);

    /* 优化后：使用拆分后的结构体 */
    start = clock();
    for (i = 1; i <= N; i++) {
        P_x[i].d_x = P_x[i].t_x * P_x[i].v_x;
    }
    end = clock();
    Total_time = (double)(end - start) / CLOCKS_PER_SEC;
    printf("结构拆分优化后：%lf秒\n", Total_time);

    /* 释放内存 */
    free(P_full);
    free(P_x);

    return 0;
}

/**
 * 优化原理：
 * 1. 优化前：motion_full 结构体大小为 9 * 4 = 36 字节
 *    - 访问 t_x, v_x, d_x 时，需要加载整个结构体
 *    - 浪费了 24 字节的缓存空间
 *
 * 2. 优化后：motion_x 结构体大小为 3 * 4 = 12 字节
 *    - 访问 t_x, v_x, d_x 时，只加载需要的数据
 *    - 缓存利用率更高
 *
 * 优化建议：
 * 1. 分析数据访问模式，确定哪些成员经常一起访问
 * 2. 将经常一起访问的成员放在同一个结构体中
 * 3. 将不经常访问的成员放在另一个结构体中
 * 4. 使用指针关联不同的结构体
 *
 * 适用场景：
 * 1. 结构体成员访问模式不均匀
 * 2. 部分成员很少被访问
 * 3. 结构体数组足够大，缓存空间成为瓶颈
 *
 * 注意事项：
 * 1. 结构体拆分会增加代码复杂度
 * 2. 需要维护多个结构体之间的关系
 * 3. 可能增加内存分配和释放的开销
 */
