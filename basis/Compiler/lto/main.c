// 链接时优化 (Link-Time Optimization, LTO)
//
// 什么是链接时优化？
//   在链接阶段对所有编译单元进行全局优化，突破单文件编译的限制。
//   普通编译时，每个 .c 文件独立编译为 .o 文件，链接器只做符号解析。
//   LTO 保留 LLVM IR 到链接阶段，链接器可以看到所有代码，进行全局优化。
//
// LTO 的作用:
//   1. 跨文件内联 — main.c 中调用 foo1()，LTO 可将 a.c 中的 foo1() 内联
//   2. 全局死代码删除 — 未被调用的函数（如 foo2()）可被删除
//   3. 全局常量传播 — 跨文件的常量可被传播和折叠
//   4. 更好的过程间优化 — 编译器可以看到整个程序的调用关系
//
// 编译命令:
//   普通编译:
//     clang -c main.c -o main.o
//     clang -c a.c -o a.o
//     clang main.o a.o -o main
//
//   LTO 编译:
//     clang -flto -c main.c -o main.o
//     clang -flto -c a.c -o a.o
//     clang -flto main.o a.o -o main
//
// 选项:
//   -flto  启用链接时优化（编译和链接阶段都要加）

#include <stdio.h>

#include "a.h"

// foo4() 被 a.c 中的 foo3() 调用
void foo4(void) { printf("Hi\n"); }

// main() 调用 foo1()，LTO 可将 foo1() 内联到此处
int main() { return foo1(); }
