// Pragma 浮点优化控制 (Pragma Floating-point Optimization)
//
// 本例展示如何通过 pragma 指令控制浮点运算优化。
//
// 常用 pragma 指令:
//   #pragma clang fp contract(fast)    — 允许融合乘加（FMA）
//   #pragma clang fp contract(on)      — 允许融合乘加，但保持精度
//   #pragma clang fp contract(off)     — 禁止融合乘加
//
// 融合乘加 (Fused Multiply-Add, FMA):
//   将乘法和加法合并为一条指令：a * b + c → fma(a, b, c)
//   优点：更快（一条指令）、更精确（只舍入一次）
//   缺点：结果可能与分开计算不同（精度差异）
//
// 本例中的运算:
//   a = b[i] * c[i];    // 乘法
//   d[i] += a;           // 加法
//
// 使用 #pragma clang fp contract(fast) 后:
//   d[i] = fma(b[i], c[i], d[i]);  // 融合为一条 FMA 指令
//
// 编译命令:
//   clang -O1 pragma_fp.cpp -o pragma_fp
//
// 选项:
//   -O1  启用优化（pragma 需要优化支持）

#include <stdio.h>

int main() {
    int N = 1024;
    int b[N], c[N], d[N];

    for (int i = 0; i < N; i++) {
        b[i] = i;
        c[i] = i + 1;
    }

    for (int i = 0; i < N; i++) {
        // 允许融合乘加，将乘法和加法合并为 FMA 指令
        #pragma clang fp contract(fast)
        int a = b[i] * c[i];
        d[i] += a;
    }

    printf("%d", d[3]);
    return 0;
}
