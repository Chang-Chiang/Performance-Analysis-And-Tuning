// 过程间优化 (Interprocedural Optimization, IPO)
// 内联优化 (Inline Optimization)
//
// 什么是过程间优化？
//   跨越函数边界进行优化，分析多个函数之间的调用关系，
//   找到单个函数内无法发现的优化机会。
//
// 什么是内联优化？
//   将被调用函数的函数体直接展开到调用处，消除函数调用开销。
//
// 内联优化的作用:
//   1. 消除调用开销 — 省去参数传递、压栈、跳转、返回等操作
//   2. 便于后续优化 — 内联后编译器可以看到完整代码，进行常量折叠、死代码删除等
//   3. 减少函数调用层数 — 深层调用链被展平，减少栈帧创建
//
// 优化前:
//   int add(int a, int b) { return a + b; }
//   main() {
//       add(16, 15);  // 调用开销：压栈、跳转、返回
//   }
//
// 优化后:
//   main() {
//       int result = 16 + 15;  // 函数体被内联，常量折叠为 31
//   }
//
// 生成 LLVM IR
//   clang -emit-llvm -S -Xclang -disable-O0-optnone ipo.cpp -o ipo.ll
//
// 执行内联优化
//   opt -passes='mem2reg,inline' ipo.ll -S -o ipo_opt.ll
//
// 选项:
//   -emit-llvm             生成 LLVM IR 而非本地机器码
//   -S                     输出文本格式
//   -Xclang -disable-O0-optnone  禁止添加 optnone 属性
//   -passes='mem2reg,inline'     先提升内存访问为寄存器，再执行内联

#include <stdio.h>

int add(int a, int b) {
    return a + b;
}

int main() {
    printf("hello!");
    add(16, 15);  // 内联后，函数调用被替换为函数体
    return 0;
}
