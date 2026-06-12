// If 转换 (If-conversion)
//
// 什么是 If 转换？
//   将条件分支语句转换为顺序执行的条件赋值语句，
//   从而把控制依赖转换成数据依赖，消除分支跳转。
//
// If 转换的作用：
//   1. 消除分支跳转 — 避免分支预测失败的惩罚
//   2. 提高流水线效率 — 无分支代码更容易流水线化
//   3. 便于向量化 — 消除分支后可以使用 SIMD 指令
//
// 示例分析：
//   优化前（条件分支）：
//     for (i = 0; i < N; i++) {
//         if (i * 2 > 2) {
//             c[i] = C0;
//         } else if (i * 2 < 2) {
//             c[i] = C1;
//         } else {
//             d[i] = D0;
//         }
//     }
//
//   优化后（If 转换）：
//     for (i = 0; i < N; i++) {
//         c[i] = (i * 2 > 2) ? C0 : c[i];
//         c[i] = (!(i * 2 > 2) && (i * 2 < 2)) ? C1 : c[i];
//         d[i] = (!(i * 2 > 2) && !(i * 2 < 2)) ? D0 : d[i];
//     }
//
// 编译命令：
//   g++ -O2 if_conversion.cpp -o if_conversion

#include <stdio.h>

#define N 10

// 示例 1：优化前（多分支判断）
void example_1_before() {
    int c[N], d[N];
    int C0 = 1, C1 = 2, D0 = 3;

    for (int i = 0; i < N; i++) {
        c[i] = i;
        d[i] = i + 1;
    }

    for (int i = 0; i < N; i++) {
        if (i * 2 > 2) {
            c[i] = C0;
        } else if (i * 2 < 2) {
            c[i] = C1;
        } else {
            d[i] = D0;
        }
    }

    printf("Example 1 (before): c[0]=%d, d[0]=%d\n", c[0], d[0]);
}

// 示例 2：优化后（If 转换）
void example_1_after() {
    int c[N], d[N];
    int C0 = 1, C1 = 2, D0 = 3;

    for (int i = 0; i < N; i++) {
        c[i] = i;
        d[i] = i + 1;
    }

    for (int i = 0; i < N; i++) {
        c[i] = (i * 2 > 2) ? C0 : c[i];
        c[i] = (!(i * 2 > 2) && (i * 2 < 2)) ? C1 : c[i];
        d[i] = (!(i * 2 > 2) && !(i * 2 < 2)) ? D0 : d[i];
    }

    printf("Example 1 (after):  c[0]=%d, d[0]=%d\n", c[0], d[0]);
}

// 示例 3：简单分支优化前
void example_2_before() {
    int a[N];
    int C0 = 1, C1 = 2;

    for (int i = 0; i < N; i++) {
        a[i] = 0;
    }

    for (int i = 0; i < N; i++) {
        if (i * 2 > 2) {
            a[i] = C0;
        } else {
            a[i] = C1;
        }
    }

    printf("Example 2 (before): a[0]=%d\n", a[0]);
}

// 示例 4：简单分支优化后（If 转换）
void example_2_after() {
    int a[N];
    int C0 = 1, C1 = 2;

    for (int i = 0; i < N; i++) {
        a[i] = 0;
    }

    for (int i = 0; i < N; i++) {
        a[i] = (i * 2 > 2) ? C0 : C1;
    }

    printf("Example 2 (after):  a[0]=%d\n", a[0]);
}

int main() {
    example_1_before();
    example_1_after();
    example_2_before();
    example_2_after();
    return 0;
}
