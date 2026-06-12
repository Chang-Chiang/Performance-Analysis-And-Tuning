/**
 * 循环分块优化示例 - 提高寄存器重用率
 *
 * 原理：
 * 循环分块（Loop Tiling/Blocking）通过将大循环分解为多个小循环，
 * 使得每个小循环处理的数据块能够完全放入寄存器或缓存中，
 * 从而提高数据的重用率，减少内存访问次数。
 *
 * 本例展示了一个二维数组的累加操作：
 * 优化前：按行遍历，每次迭代都需要访问 A[i]
 * 优化后：将外层循环分块，使得内层循环处理的数据块能够放入寄存器
 *
 * 编译指令：
 * gcc -O2 -o loop_tiling_register_reuse loop_tiling_register_reuse.c
 *
 * 运行：
 * ./loop_tiling_register_reuse
 */

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

/* 分块大小，需要根据目标架构的寄存器数量和缓存大小调整 */
int Ti = 3;

/* 最小值宏 */
#define min(a,b) ((a)<(b)?(a):(b))

/**
 * 循环分块优化函数
 * @param A 输出数组
 * @param B 输入二维数组（以一维形式存储）
 * @param NI 数组 A 的长度
 * @param NJ 数组 B 的列数
 *
 * 分块策略：
 * 将外层循环按 Ti 大小分块，使得内层循环每次处理 Ti 个元素
 * 这样 A[ii..ii+Ti-1] 可以保存在寄存器中，提高重用率
 */
void func(float* A, float* B, int NI, int NJ) {
    /* 外层循环：按 Ti 大小分块 */
    for (int ii = 0; ii < NI; ii += Ti) {
        /* 中间层循环：遍历 B 的列 */
        for (int j = 0; j < NJ; ++j) {
            /* 内层循环：处理当前块的元素
             * 这里 A[ii..ii+Ti-1] 可以保存在寄存器中 */
            for (int i = ii; i < min(ii + Ti, NI); ++i) {
                A[i] += B[j * NI + i];  /* 累加操作 */
            }
        }
    }
}

int main() {
    int i;
    float *A, *B;
    int na = 6, nb = 12;

    /* 分配内存 */
    A = (float*)malloc(na * sizeof(float));
    B = (float*)malloc(100 * sizeof(float));

    /* 初始化数组 */
    for (i = 0; i < na; i++) {
        A[i] = rand() % 10;
    }
    for (i = 0; i < 100; i++) {
        B[i] = rand() % 10;
    }

    /* 打印初始值 */
    printf("数组A的初始值：\n");
    for (int i = 0; i < na; i++) {
        printf(" %.5f ", A[i]);
    }

    /* 执行循环分块优化 */
    func(A, B, na, nb);

    /* 打印结果 */
    printf("\n循环分块优化后数组A的值：\n");
    for (int i = 0; i < na; i++) {
        printf(" %.5f ", A[i]);
    }
    printf("\n");

    /* 释放内存 */
    free(A);
    free(B);
    return 0;
}

/**
 * 分块大小选择建议：
 * 1. Ti 应该小于或等于可用寄存器数量
 * 2. 对于浮点运算，现代 CPU 通常有 8-16 个浮点寄存器
 * 3. Ti = 3~8 是一个合理的范围
 * 4. 可以通过实验找到最优的分块大小
 */
