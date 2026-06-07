// 循环剥离 (Loop Peeling)
//
// 什么是循环剥离？
//   将循环的前几次或后几次迭代"剥离"出来，作为独立代码执行，
//   剩余部分仍用循环处理。
//
// 循环剥离的作用:
//   1. 内存对齐 — 剥离若干次迭代后，剩余循环的起始地址对齐到向量边界，
//      可使用对齐加载指令（aligned load），比非对齐加载更高效
//   2. 消除依赖 — 前几次迭代可能有循环携带依赖，剥离后剩余循环可向量化
//   3. 边界处理 — 循环尾部不足一个向量宽度的迭代被剥离，剩余循环可完全向量化
//
// 本例中的循环访问 a[j+2]、b[j+2]，偏移量为 2：
//   for (j = 0; j < N; j++) { sum += a[j+2] + b[j+2]; }
//
// 优化前: 从 a[2] 开始访问，不满足向量对齐要求
//   a[0] a[1] a[2] a[3] | a[4] a[5] a[6] a[7] | ...
//                  ↑ 起始位置，跨越两个向量，无法对齐加载
//
// 优化后: 从 a[4] 开始访问，满足向量对齐要求
//   sum += a[2] + b[2];                    // 剥离 j=0
//   sum += a[3] + b[3];                    // 剥离 j=1
//   for (j = 2; j < N; j++) {              // 从 a[4] 开始，对齐向量边界，可向量化
//       sum += a[j+2] + b[j+2];
//   }
//
// 生成 LLVM IR
//   clang -emit-llvm -S -Xclang -disable-O0-optnone loop_peel.cpp -o loop_peel.ll
//
// 执行循环剥离优化
//   opt -passes='mem2reg,loop(loop-peel)' loop_peel.ll -S -o loop_peel_opt.ll
//
// 选项:
//   -emit-llvm             生成 LLVM IR 而非本地机器码
//   -S                     输出文本格式
//   -Xclang -disable-O0-optnone  禁止添加 optnone 属性
//   -passes='mem2reg,loop(loop-peel)'  先提升内存访问为寄存器，再执行循环剥离

#include <stdio.h>

#define N 1280

int main() {
    int sum = 0;
    int a[N], b[N];

    for (int i = 0; i < N; i++) {
        a[i] = i;
        b[i] = i + 3;
    }

    // 循环剥离：将前几次迭代移出循环，减少循环开销
    for (int j = 0; j < N; j++) {
        sum = sum + a[j + 2] + b[j + 2];
    }

    printf("sum = %d", sum);
    return 0;
}
