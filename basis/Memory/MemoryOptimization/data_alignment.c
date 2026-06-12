/**
 * 数据对齐优化示例
 *
 * 原理：
 * 数据对齐（Data Alignment）是指数据在内存中的地址是某个值的倍数。
 * 对齐的数据可以提高内存访问效率，因为 CPU 通常以对齐的方式访问内存。
 *
 * 本例展示了结构体的对齐问题：
 * - 结构体成员的排列顺序会影响结构体的大小
 * - 编译器会在成员之间插入填充字节以满足对齐要求
 * - 合理排列成员顺序可以减少填充，节省内存
 *
 * 编译指令：
 * gcc -O2 -o data_alignment data_alignment.c
 *
 * 运行：
 * ./data_alignment
 */

#include <stdio.h>

/**
 * 优化前的结构体
 * 成员排列顺序不合理，导致大量填充
 *
 * 内存布局：
 * - char a: 1 字节 + 7 字节填充（对齐到 8 字节）
 * - long b: 8 字节
 * - int c: 4 字节 + 4 字节填充（对齐到 8 字节）
 * 总大小：24 字节
 */
struct AlignA {
    char a;    /* 1 字节 */
    long b;    /* 8 字节 */
    int c;     /* 4 字节 */
};

/**
 * 优化后的结构体
 * 成员排列顺序合理，减少填充
 *
 * 内存布局：
 * - char a: 1 字节
 * - int c: 4 字节（3 字节填充）
 * - long b: 8 字节
 * 总大小：16 字节
 */
struct AlignB {
    char a;    /* 1 字节 */
    int c;     /* 4 字节 */
    long b;    /* 8 字节 */
};

int main() {
    char a;
    long b;
    int c;

    printf("基本类型大小：\n");
    printf("  char:  %lu 字节\n", sizeof(char));
    printf("  int:   %lu 字节\n", sizeof(int));
    printf("  long:  %lu 字节\n", sizeof(long));
    printf("\n");

    printf("AlignA 结构体（优化前）：\n");
    printf("  各成员大小之和：%lu 字节\n", sizeof(a) + sizeof(b) + sizeof(c));
    printf("  结构体实际大小：%lu 字节\n", sizeof(struct AlignA));
    printf("  浪费的填充字节：%lu 字节\n", sizeof(struct AlignA) - (sizeof(a) + sizeof(b) + sizeof(c)));
    printf("\n");

    printf("AlignB 结构体（优化后）：\n");
    printf("  各成员大小之和：%lu 字节\n", sizeof(a) + sizeof(b) + sizeof(c));
    printf("  结构体实际大小：%lu 字节\n", sizeof(struct AlignB));
    printf("  浪费的填充字节：%lu 字节\n", sizeof(struct AlignB) - (sizeof(a) + sizeof(b) + sizeof(c)));

    return 0;
}

/**
 * 对齐规则：
 * 1. 每个成员的地址必须是其大小的整数倍
 * 2. 结构体的总大小必须是最大成员大小的整数倍
 * 3. 编译器会在成员之间插入填充字节以满足对齐要求
 *
 * 优化建议：
 * 1. 按成员大小从大到小排列（long -> int -> char）
 * 2. 或者按成员大小从小到大排列（char -> int -> long）
 * 3. 避免混合大小的成员交替排列
 * 4. 使用 pragma pack 指令可以改变对齐方式（但可能影响性能）
 *
 * 对齐对性能的影响：
 * 1. 对齐的内存访问通常更快
 * 2. 非对齐的内存访问可能需要两次内存访问
 * 3. 某些架构（如 ARM）对非对齐访问有性能惩罚
 * 4. 对齐还可以提高缓存利用率
 */
