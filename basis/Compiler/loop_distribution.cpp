// 循环分布 (Loop Distribution / Loop Fission)
//
// 什么是循环分布？
//   将一个包含多条语句的循环拆分成多个循环，每个循环包含部分语句。
//
// 循环分布的作用:
//   1. 分离依赖 — 将有依赖的语句和无依赖的语句分开，无依赖的循环可向量化
//   2. 提升缓存利用率 — 每个循环只访问需要的数据，减少缓存污染
//   3. 便于其他优化 — 拆分后的循环更容易被展开、向量化等
//
// 本例中的循环:
//   for (i = 1; i < N; i++) {
//       A[i] = i;            // 无依赖，可向量化
//       B[i] = 2 + B[i];    // 无依赖，可向量化
//       C[i] = 3 + C[i-1];  // 有循环携带依赖，无法向量化
//   }
//
// 优化后:
//   for (i = 1; i < N; i++) { A[i] = i; }         // 可向量化
//   for (i = 1; i < N; i++) { B[i] = 2 + B[i]; }  // 可向量化
//   for (i = 1; i < N; i++) { C[i] = 3 + C[i-1]; } // 有依赖，标量执行
//
// 生成 LLVM IR
//   clang -emit-llvm -S -Xclang -disable-O0-optnone loop_distribution.cpp -o loop_distribution.ll
//
// 执行循环分布优化
//   opt -passes='mem2reg,loop(loop-distribute)' loop_distribution.ll -S -o loop_distribution_opt.ll
//
// 选项:
//   -emit-llvm             生成 LLVM IR 而非本地机器码
//   -S                     输出文本格式
//   -Xclang -disable-O0-optnone  禁止添加 optnone 属性
//   -passes='mem2reg,loop(loop-distribute)'  先提升内存访问为寄存器，再执行循环分布

#include <stdio.h>
#include <stdlib.h>

#define N 1280

int main() {
    int A[N], B[N], C[N];
    int i;

    for (i = 0; i < N; i++) {
        B[i] = rand();
        C[i] = rand();
    }

    for (i = 1; i < N; i++) {
        A[i] = i;            // 无依赖，可向量化
        B[i] = 2 + B[i];    // 无依赖，可向量化
        C[i] = 3 + C[i-1];  // 有循环携带依赖，无法向量化
    }

    for (i = 0; i < N; i++) {
        printf("%d", B[i]);
        printf("%d", A[i]);
        printf("%d", C[i]);
    }

    return 0;
}
