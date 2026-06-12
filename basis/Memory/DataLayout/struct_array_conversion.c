/**
 * 结构体数组转换优化示例
 *
 * 原理：
 * 结构体数组转换（Struct Array Conversion）通过将结构体数组（AoS）
 * 转换为数组结构体（SoA），提高数据的局部性和缓存命中率。
 *
 * 优化前（AoS - Array of Structures）：
 * - 结构体数组：motion P[N]
 * - 内存布局：[t_x0, v_x0, d_x0, t_y0, v_y0, d_y0, ...]
 * - 访问 P[i].t_x, P[i].v_x 时，数据在内存中连续
 * - 但访问 P[0].t_x, P[1].t_x, P[2].t_x 时，数据不连续
 *
 * 优化后（SoA - Structure of Arrays）：
 * - 数组结构体：motion_x P_1
 * - 内存布局：[t_x0, t_x1, t_x2, ...], [v_x0, v_x1, v_x2, ...], [d_x0, d_x1, d_x2, ...]
 * - 访问 P_1.t_x[0], P_1.t_x[1], P_1.t_x[2] 时，数据连续
 * - 提高缓存命中率和向量化效率
 *
 * 编译指令：
 * gcc -O2 -o struct_array_conversion struct_array_conversion.c
 *
 * 运行：
 * ./struct_array_conversion
 */

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <malloc.h>

#define N 100000

/**
 * 优化前的结构体（AoS）
 * 所有维度的数据在同一个结构体中
 */
typedef struct {
    float t_x, t_y, t_z;  /* 时间分量 */
    float v_x, v_y, v_z;  /* 速度分量 */
    float d_x, d_y, d_z;  /* 位移分量 */
} motion_aos;

/**
 * 优化后的结构体（SoA）
 * 每个维度的数据在独立的数组中
 */
typedef struct {
    float Pt_x[N];  /* 所有元素的 t_x */
    float Pv_x[N];  /* 所有元素的 v_x */
    float Pd_x[N];  /* 所有元素的 d_x */
} motion_soa;

int main() {
    int i;
    clock_t start, end;
    double Total_time;

    /* 分配内存 */
    motion_aos* P_aos = (motion_aos*)malloc(N * sizeof(motion_aos));
    motion_soa P_soa;

    /* 初始化数据 */
    for (i = 0; i < N; i++) {
        P_aos[i].t_x = rand() % 10;
        P_aos[i].v_x = rand() % 10;
        P_soa.Pt_x[i] = rand() % 10;
        P_soa.Pv_x[i] = rand() % 10;
        P_soa.Pd_x[i] = 0;
    }

    /* 优化前：使用 AoS 访问模式
     * 问题：访问 P[i].t_x, P[i+1].t_x 时，数据间隔为整个结构体大小 */
    start = clock();
    for (i = 1; i <= N; i++) {
        P_aos[i].d_x = P_aos[i].t_x * P_aos[i].v_x;
    }
    end = clock();
    Total_time = (double)(end - start) / CLOCKS_PER_SEC;
    printf("结构体数组转为数组结构体优化前：%lf秒\n", Total_time);

    /* 优化后：使用 SoA 访问模式
     * 优化：访问 P_soa.t_x[i], P_soa.t_x[i+1] 时，数据连续存放 */
    start = clock();
    for (i = 1; i <= N; i++) {
        P_soa.Pd_x[i] = P_soa.Pt_x[i] * P_soa.Pv_x[i];
    }
    end = clock();
    Total_time = (double)(end - start) / CLOCKS_PER_SEC;
    printf("结构体数组转为数组结构体优化后：%lf秒\n", Total_time);

    /* 释放内存 */
    free(P_aos);

    return 0;
}

/**
 * AoS vs SoA 对比：
 *
 * AoS（Array of Structures）：
 * - 优点：访问单个元素的所有属性时，数据局部性好
 * - 缺点：访问多个元素的同一属性时，数据不连续
 * - 适用场景：面向对象编程，单个元素的处理
 *
 * SoA（Structure of Arrays）：
 * - 优点：访问多个元素的同一属性时，数据连续
 * - 缺点：访问单个元素的所有属性时，数据不连续
 * - 适用场景：数据并行处理，向量化计算
 *
 * 选择建议：
 * 1. 如果经常需要访问单个元素的所有属性，使用 AoS
 * 2. 如果经常需要访问多个元素的同一属性，使用 SoA
 * 3. 如果需要向量化，使用 SoA
 * 4. 如果需要缓存友好，根据访问模式选择
 *
 * 向量化优势：
 * - SoA 布局更适合 SIMD 向量化
 * - 连续的数据可以一次性加载到向量寄存器中
 * - 提高计算密度和性能
 */
