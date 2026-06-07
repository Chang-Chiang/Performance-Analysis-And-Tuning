// 死代码删除 (Dead Code Elimination, DCE)
//
// 什么是死代码删除？
//   删除程序中不会影响程序结果的代码，包括：
//   1. 死赋值 — 变量赋值后从未被使用
//   2. 死分支 — 条件永远为 false 的分支
//   3. 不可达代码 — 永远不会执行到的代码
//
// 死代码删除的作用:
//   1. 减少代码体积 — 删除无用指令，减小程序大小
//   2. 减少执行时间 — 不再执行无意义的计算
//   3. 减少寄存器压力 — 释放被死变量占用的寄存器
//   4. 为其他优化创造条件 — 删除死代码后，其他 pass 可能发现新的优化机会
//
// 本例中的死代码分析:
//   a = rand();          // a 被 S1 使用
//   b = rand();          // b 被 S1 使用
//   c = a + b * 3;  // S1: c 被 return 使用
//   b = a;          // S2: b 赋值后再无使用，是死代码
//   return c;
//
// 优化后:
//   a = rand();
//   b = rand();
//   c = a + b * 3;
//   return c;           // S2 被删除
//
// 生成 LLVM IR（不带 optnone 属性）
//   clang -emit-llvm -S -Xclang -disable-O0-optnone dce.cpp -o dce.ll
//
// 执行死代码删除优化
//   opt -passes='mem2reg,dce' dce.ll -S -o dce_opt.ll
//
// 选项:
//   -emit-llvm             生成 LLVM IR 而非本地机器码
//   -S                     输出文本格式
//   -Xclang -disable-O0-optnone  禁止添加 optnone 属性, optnone 属性会阻止 LLVM IR 被优化
//   -passes='mem2reg,dce'  先提升内存访问为寄存器，再删除死代码

#include <stdio.h>
#include <stdlib.h>

int main() {
    int a, b, c;
    a = rand();
    b = rand();
    c = a + b * 3; // S1: c 被使用（return c）
    b = a;         // S2: b 赋值后再无使用，是死代码，可删除
    return c;
}
