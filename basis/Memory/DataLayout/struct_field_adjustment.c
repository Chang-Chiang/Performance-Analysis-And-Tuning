/**
 * 结构属性域调整优化示例
 *
 * 原理：
 * 结构属性域调整（Struct Field Adjustment）通过重新排列结构体成员的顺序，
 * 使相关数据在内存中连续存放，提高缓存命中率。
 *
 * 优化前：
 * - 结构体成员按维度分组：t_x, t_y, t_z, v_x, v_y, v_z, d_x, d_y, d_z
 * - 访问 x 维度的数据时，t_x, v_x, d_x 在内存中不连续
 * - 每次访问都可能产生缓存未命中
 *
 * 优化后：
 * - 结构体成员按属性分组：t_x, v_x, d_x, t_y, v_y, d_y, t_z, v_z, d_z
 * - 访问 x 维度的数据时，t_x, v_x, d_x 在内存中连续
 * - 提高缓存命中率
 *
 * 编译指令：
 * gcc -O2 -o struct_field_adjustment struct_field_adjustment.c
 *
 * 运行：
 * ./struct_field_adjustment
 */

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <malloc.h>

/**
 * 优化前的结构体
 * 成员按维度分组，同一维度的数据在内存中不连续
 *
 * 内存布局：
 * t_x, t_y, t_z, v_x, v_y, v_z, d_x, d_y, d_z
 *
 * 问题：访问 x 维度的数据时，t_x, v_x, d_x 分散在不同位置
 */
typedef struct {
    float t_x, t_y, t_z;  /* 时间分量 */
    float v_x, v_y, v_z;  /* 速度分量 */
    float d_x, d_y, d_z;  /* 位移分量 */
} motion_before;

/**
 * 优化后的结构体
 * 成员按属性分组，同一维度的数据在内存中连续
 *
 * 内存布局：
 * t_x, v_x, d_x, t_y, v_y, d_y, t_z, v_z, d_z
 *
 * 优化：访问 x 维度的数据时，t_x, v_x, d_x 连续存放
 */
typedef struct {
    float t_x, v_x, d_x;  /* x 维度：时间、速度、位移 */
    float t_y, v_y, d_y;  /* y 维度：时间、速度、位移 */
    float t_z, v_z, d_z;  /* z 维度：时间、速度、位移 */
} motion_after;

int main() {
    int N = 10000000;
    int i;
    clock_t start, end;
    double Total_time;

    /* 分配内存 */
    motion_before* P_before = (motion_before*)malloc(N * sizeof(motion_before));
    motion_after* P_after = (motion_after*)malloc(N * sizeof(motion_after));

    /* 初始化数据 */
    for (i = 0; i < N; i++) {
        P_before[i].t_x = rand() % 10;
        P_before[i].v_x = rand() % 10;
        P_after[i].t_x = rand() % 10;
        P_after[i].v_x = rand() % 10;
    }

    /* 优化前：访问 x 维度的数据 */
    start = clock();
    for (i = 1; i <= N; i++) {
        P_before[i].d_x = P_before[i].t_x * P_before[i].v_x;
    }
    end = clock();
    Total_time = (double)(end - start) / CLOCKS_PER_SEC;
    printf("结构属性域优化前：%lf秒\n", Total_time);

    /* 优化后：访问 x 维度的数据 */
    start = clock();
    for (i = 1; i <= N; i++) {
        P_after[i].d_x = P_after[i].t_x * P_after[i].v_x;
    }
    end = clock();
    Total_time = (double)(end - start) / CLOCKS_PER_SEC;
    printf("结构属性域优化后：%lf秒\n", Total_time);

    /* 释放内存 */
    free(P_before);
    free(P_after);

    return 0;
}

/**
 * 优化原理：
 * 1. 优化前：t_x, v_x, d_x 之间间隔 3 个 float（12 字节）
 *    - 访问 t_x 后，v_x 可能不在同一个缓存行中
 *    - 需要加载多个缓存行
 *
 * 2. 优化后：t_x, v_x, d_x 连续存放
 *    - 访问 t_x 后，v_x 和 d_x 已经在同一个缓存行中
 *    - 只需要加载一个缓存行
 *
 * 优化建议：
 * 1. 分析数据访问模式，确定哪些成员经常一起访问
 * 2. 将经常一起访问的成员放在相邻位置
 * 3. 考虑缓存行大小，确保相关数据在同一个缓存行中
 * 4. 使用结构体数组时，考虑是否需要转置为数组结构体
 *
 * 适用场景：
 * 1. 结构体成员经常按特定模式访问
 * 2. 结构体数组足够大，缓存未命中成为瓶颈
 * 3. 访问模式固定，值得进行优化
 */
