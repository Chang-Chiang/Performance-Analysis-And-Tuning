// 无法向量化的循环 (Loop Vectorization Unable)
//
// 什么情况下循环无法向量化？
//   1. 循环携带依赖 — 迭代之间存在数据依赖，无法并行执行
//   2. 非连续内存访问 — 跨步访问或间接访问，无法使用连续加载指令
//   3. 循环次数不确定 — 编译器无法确定迭代次数，无法生成向量代码
//   4. 函数调用 — 循环体内有无法内联的函数调用
//
// 本例中的两个循环:
//   for (i = 0; i < N; i++) {
//       sum = sum + a[i];    // 可向量化：无循环携带依赖，连续访问
//   }
//
//   for (i = 0; i < N; i++) {
//       b[i+1] = b[i] + b[i+2];  // 无法向量化：b[i+1] 依赖 b[i]（循环携带依赖）
//   }
//
// 生成 LLVM IR
//   clang -emit-llvm -S -Xclang -disable-O0-optnone loop_vectorization_unable.cpp -o loop_vectorization_unable.ll
//
// 查看向量化报告
//   opt -passes='mem2reg,loop-vectorize' -pass-remarks-missed=loop-vectorize loop_vectorization_unable.ll -S -o /dev/null
//
// 选项:
//   -emit-llvm             生成 LLVM IR 而非本地机器码
//   -S                     输出文本格式
//   -Xclang -disable-O0-optnone  禁止添加 optnone 属性
//   -pass-remarks-missed   输出优化失败的原因

#include <stdio.h>

#define N 128

int main() {
    int sum = 0;
    int a[N], b[N];

    for (int i = 0; i < N; i++) {
        a[i] = i;
        b[i] = i + 1;
    }

    // 可向量化：无循环携带依赖，连续内存访问
    for (int i = 0; i < N; i++) {
        sum = sum + a[i];
    }

    // 无法向量化：b[i+1] 依赖 b[i]，存在循环携带依赖
    for (int i = 0; i < N; i++) {
        b[i + 1] = b[i] + b[i + 2];
    }

    printf("sum = %d", sum);
    return 0;
}
