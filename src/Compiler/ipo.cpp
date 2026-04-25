// ipo, interprocedural optimization, 过程间优化

// 内联优化, 消除函数调用开销
// 生成 LLVM IR代码
// clang ipo.cpp -emit-llvm -S -O1 -o ipo.ll
// 内联优化后的 LLVM IR代码
// opt ipo.ll -passes=inline -S -o ipo_opt.ll

#include <stdio.h>

int add(int a, int b) {
    return a+b;
}

int main() {
    printf("hello!");
    add(16, 15);
    return 0;
}

