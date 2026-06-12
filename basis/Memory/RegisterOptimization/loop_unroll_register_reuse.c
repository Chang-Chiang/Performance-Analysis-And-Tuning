/**
 * 循环展开优化示例 - 提高寄存器重用率
 *
 * 原理：
 * 循环展开（Loop Unrolling）通过将循环体复制多次，减少循环控制开销，
 * 并允许编译器更好地利用寄存器和指令级并行性。
 *
 * 本例展示了将内层循环展开 4 次的效果：
 * 优化前：每次迭代处理 1 个元素
 * 优化后：每次迭代处理 4 个元素，减少循环控制开销
 *
 * 编译指令：
 * gcc -O2 -o loop_unroll_register_reuse loop_unroll_register_reuse.c
 *
 * 运行：
 * ./loop_unroll_register_reuse
 */

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

/* 分块大小 */
int Ti = 3;

/* 最小值宏 */
#define min(a,b) ((a)<(b)?(a):(b))

/**
 * 循环展开 4 次的优化函数
 * @param A 输出数组
 * @param B 输入二维数组（以一维形式存储）
 * @param NI 数组 A 的长度
 * @param NJ 数组 B 的列数
 *
 * 展开策略：
 * 将内层循环展开 4 次，每次迭代处理 4 个元素
 * 这样可以减少循环控制开销，并允许编译器更好地利用寄存器
 */
void func_unroll4(float* A, float* B, int NI, int NJ) {
    /* 外层循环：按 Ti 大小分块 */
    for (int ii = 0; ii < NI; ii += Ti) {
        /* 中间层循环：按 4 步长遍历（展开 4 次） */
        for (int j = 0; j < NJ; j += 4) {
            /* 内层循环：处理当前块的元素 */
            for (int i = ii; i < min(ii + Ti, NI); ++i) {
                /* 展开 4 次：连续累加 4 个元素
                 * 编译器可以将这些操作流水线化 */
                A[i] += B[j * NI + i];
                A[i] += B[(j + 1) * NI + i];
                A[i] += B[(j + 2) * NI + i];
                A[i] += B[(j + 3) * NI + i];
            }
        }
    }
}

int main() {
    float *A, *B;
    int i, na = 6, nb = 12;

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

    /* 执行循环展开优化 */
    func_unroll4(A, B, 6, 12);

    /* 打印结果 */
    printf("\n循环展开四次运算后数组A的值：\n");
    for (i = 0; i < na; i++) {
        printf(" %.5f ", A[i]);
    }
    printf("\n");

    /* 释放内存 */
    free(A);
    free(B);
    return 0;
}

/**
 * 循环展开的优点：
 * 1. 减少循环控制开销（比较、跳转指令）
 * 2. 提高指令级并行性
 * 3. 更好地利用寄存器
 * 4. 减少分支预测失败
 *
 * 循环展开的缺点：
 * 1. 代码体积增大
 * 2. 可能导致指令缓存未命中
 * 3. 需要处理边界情况（循环次数不是展开因子的倍数）
 *
 * 展开因子选择建议：
 * 1. 通常选择 2, 4, 8 等 2 的幂次
 * 2. 需要考虑目标架构的寄存器数量
 * 3. 需要考虑指令缓存大小
 * 4. 可以通过实验找到最优的展开因子
 */
