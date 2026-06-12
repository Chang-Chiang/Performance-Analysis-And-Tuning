// 指令流水优化 (Instruction Pipeline Optimization)
//
// 什么是指令流水？
//   将指令执行分为多个阶段（取指、译码、执行、访存、写回），
//   多条指令可以同时处于不同的执行阶段，提高 CPU 利用率。
//
// 流水线优化的作用：
//   1. 提高指令吞吐量 — 多条指令并行执行
//   2. 减少空闲周期 — 流水线充满后每个周期都有指令完成
//   3. 提升 CPU 利用率 — 充分利用 CPU 的各个功能单元
//
// 控制相关优化方法：
//   1. 控制语句外提 — 将循环不变量的判断条件外提到循环外
//      主流编译器仅对最内层循环的简单控制流结构有效，
//      复杂的控制相关需要手动优化。
//
//   2. if 转换 — 将控制相关转换为数据相关
//      将条件分支语句转换为顺序执行的条件赋值语句，
//      从而把控制依赖转换成数据依赖。
//
// 示例分析：
//   优化前（分支在循环内）：
//     for (i = 0; i < N; i++) {
//         if (an > 10) {
//             a[i] = c[i];
//         } else {
//             a[i] = d[i];
//         }
//         m[i] = n[i];
//     }
//     // 每次迭代都要判断分支，影响流水线
//
//   优化后（控制语句外提）：
//     if (an > 10) {
//         for (i = 0; i < N; i++) {
//             a[i] = c[i];
//             m[i] = n[i];
//         }
//     } else {
//         for (i = 0; i < N; i++) {
//             a[i] = d[i];
//             m[i] = n[i];
//         }
//     }
//     // 分支只判断一次，循环内无分支，流水线更高效
//
// 编译命令：
//   g++ -O2 pipeline.cpp -o pipeline

#include <stdio.h>
#include <stdlib.h>

#define N 10

// 示例 1：优化前（分支在循环内）
void example_before() {
    int a[N], d[N], c[N], m[N], n[N];
    int an = rand() % 100;

    for (int i = 0; i < N; i++) {
        a[i] = 0;
        m[i] = 0;
        c[i] = i;
        d[i] = i + 1;
        n[i] = i + 2;
    }

    // 分支在循环内，每次迭代都要判断
    for (int i = 0; i < N; i++) {
        if (an > 10) {
            a[i] = c[i];
        }
        else {
            a[i] = d[i];
        }
        m[i] = n[i];
    }

    printf("Before: a[0] = %d, m[0] = %d\n", a[0], m[0]);
}

// 示例 2：优化后（分支提到循环外）
void example_after() {
    int a[N], d[N], c[N], m[N], n[N];
    int an = rand() % 100;

    for (int i = 0; i < N; i++) {
        a[i] = 0;
        m[i] = 0;
        c[i] = i;
        d[i] = i + 1;
        n[i] = i + 2;
    }

    // 分支提到循环外，只判断一次
    if (an > 10) {
        for (int i = 0; i < N; i++) {
            a[i] = c[i];
            m[i] = n[i];
        }
    }
    else {
        for (int i = 0; i < N; i++) {
            a[i] = d[i];
            m[i] = n[i];
        }
    }

    printf("After:  a[0] = %d, m[0] = %d\n", a[0], m[0]);
}

int main() {
    example_before();
    example_after();
    return 0;
}
