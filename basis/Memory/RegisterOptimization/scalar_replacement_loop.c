/**
 * 标量替换优化示例 - 循环中的寄存器重用
 *
 * 原理：
 * 在循环中，如果某个表达式的值在循环体内不变，可以将其计算结果
 * 保存到寄存器中，避免重复计算和内存访问。
 *
 * 优化前：
 * - 全局变量 x 在循环中被多次访问
 * - 每次访问 x 都需要从内存中读取
 *
 * 优化后：
 * - 将 x 的值保存到局部变量 temp 中
 * - temp 会被编译器分配到寄存器中
 * - 循环中通过 temp 访问，减少内存访问次数
 *
 * 编译指令：
 * gcc -O2 -o scalar_replacement_loop scalar_replacement_loop.c
 *
 * 运行：
 * ./scalar_replacement_loop
 */

#include <stdio.h>
#include <malloc.h>

/* 全局变量 */
float x = 5.5642;

/**
 * 优化前的函数
 * @param a 输出数组
 * @param N 数组长度
 *
 * 问题：循环中每次迭代都访问全局变量 x，导致重复内存读取
 */
void function_before(float *a, int N) {
    int i;
    float phi = 2.541, delta, alpha;

    /* 以下两行每次访问 x 都需要从内存读取 */
    delta = x * x;      /* 读取 x 两次 */
    alpha = x / 2;      /* 读取 x 一次 */

    /* 循环中每次迭代都访问全局变量 x */
    for (i = 0; i < N; i++)
        a[i] = x * phi;  /* 每次迭代都读取 x */
}

/**
 * 优化后的函数
 * @param b 输出数组
 * @param N 数组长度
 *
 * 优化：使用局部变量 temp 保存 x 的值，减少内存访问
 */
void function_after(float *b, int N) {
    int i;
    float phi = 2.541, delta, alpha, temp;

    /* 将 x 的值保存到局部变量 temp
     * temp 会被编译器分配到寄存器中 */
    temp = x;

    /* 使用 temp 代替 x，避免重复内存访问 */
    delta = temp * temp;    /* 从寄存器读取 temp */
    alpha = temp / 2;       /* 从寄存器读取 temp */

    /* 循环中使用 temp 代替 x */
    for (i = 0; i < N; i++)
        b[i] = temp * phi;  /* 从寄存器读取 temp */
}

int main() {
    float *a, *b;
    int n = 100;

    a = (float*)malloc(n * sizeof(float));
    b = (float*)malloc(n * sizeof(float));

    function_before(a, n);
    function_after(b, n);

    printf("优化前 a[0] = %f\n", a[0]);
    printf("优化后 b[0] = %f\n", b[0]);

    free(a);
    free(b);
    return 0;
}
