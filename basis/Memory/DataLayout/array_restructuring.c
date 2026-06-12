/**
 * 数组重组优化示例
 *
 * 原理：
 * 数组重组（Array Restructuring）通过将多个独立的数组合并为一个结构体数组，
 * 提高数据的局部性，减少缓存未命中。
 *
 * 优化前：
 * - 使用多个独立的数组 a, b, c, sum
 * - 每个数组在内存中独立分配
 * - 访问 a[i], b[i], c[i] 时，数据可能分散在不同的缓存行中
 *
 * 优化后：
 * - 使用结构体数组，将相关数据放在一起
 * - a[i], b[i], c[i], sum[i] 在内存中连续存放
 * - 提高缓存命中率
 *
 * 编译指令：
 * gcc -O2 -o array_restructuring array_restructuring.c
 *
 * 运行：
 * ./array_restructuring
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <malloc.h>

int main() {
    int n = 10000000, i;
    int *a, *b, *c, *sum;
    clock_t start, end;
    double Total_time;

    /* 分配内存 - 优化前：独立的数组 */
    a = (int*)malloc(n * sizeof(int));
    b = (int*)malloc(n * sizeof(int));
    c = (int*)malloc(n * sizeof(int));
    sum = (int*)malloc(n * sizeof(int));

    /* 初始化数组 */
    for (i = 0; i < n; i++) {
        a[i] = rand() % 10;
        b[i] = rand() % 10;
        c[i] = rand() % 10;
    }

    /* 优化前：使用独立数组 */
    start = clock();
    for (i = 0; i < n; i++) {
        sum[i] = a[i] + b[i] + c[i];
    }
    end = clock();
    Total_time = (double)(end - start) / CLOCKS_PER_SEC;
    printf("数组重组优化前：%lf秒\n", Total_time);

    /* 释放内存 */
    free(a);
    free(b);
    free(c);
    free(sum);

    /* 优化后：使用结构体数组 */
    typedef struct {
        int a, b, c;
        int sum;
    } arr_struct;

    arr_struct* arr;
    arr = (arr_struct*)malloc(n * sizeof(arr_struct));

    /* 初始化结构体数组 */
    for (i = 0; i < n; i++) {
        arr[i].a = rand() % 10;
        arr[i].b = rand() % 10;
        arr[i].c = rand() % 10;
    }

    /* 优化后：使用结构体数组 */
    start = clock();
    for (i = 0; i < n; i++) {
        arr[i].sum = arr[i].a + arr[i].b + arr[i].c;
    }
    end = clock();
    Total_time = (double)(end - start) / CLOCKS_PER_SEC;
    printf("数组重组优化后：%lf秒\n", Total_time);

    /* 释放内存 */
    free(arr);

    return 0;
}

/**
 * 优化原理：
 * 1. 优化前：a[i], b[i], c[i] 分散在不同的内存区域
 *    - 访问 a[i] 可能加载一个缓存行
 *    - 访问 b[i] 可能需要加载另一个缓存行
 *    - 访问 c[i] 可能需要加载第三个缓存行
 *
 * 2. 优化后：arr[i].a, arr[i].b, arr[i].c 在同一个结构体中
 *    - 访问 arr[i] 时，整个结构体会被加载到缓存行中
 *    - 后续访问 arr[i].b, arr[i].c 时，数据已经在缓存中
 *
 * 性能提升：
 * - 减少缓存未命中次数
 * - 提高缓存行利用率
 * - 对于大数据量，性能提升显著
 *
 * 适用场景：
 * 1. 多个数组的相同索引位置经常一起访问
 * 2. 数据访问模式具有局部性
 * 3. 数组大小足够大，缓存未命中成为瓶颈
 */
