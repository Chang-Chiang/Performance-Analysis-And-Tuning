// 编译流程 (Compilation Flow)
//
// ┌─────────────────────────────────────────────────────────────┐
// │ 源程序                                                       │
// │   C: test.c        C++: test.cpp                            │
// └────────────┬────────────────────────────────────────────────┘
//              │
//              ▼ 预处理 (Preprocessing)
// ┌─────────────────────────────────────────────────────────────┐
// │ 预处理文件                                                    │
// │   C: test.i        C++: test.ii                             │
// │                                                             │
// │   gcc -E test.c -o test.i        gcc -E test.cpp -o test.ii │
// │   clang -E test.c -o test.i    clang -E test.cpp -o test.ii │
// └────────────┬────────────────────────────────────────────────┘
//              │
//              ▼ 编译 (Compilation)
// ┌─────────────────────────────────────────────────────────────┐
// │ 汇编文件 (.s)                                                │
// │                                                             │
// │   gcc -S test.i -o test.s        gcc -S test.ii -o test.s   │
// │   clang -S test.i -o test.s    clang -S test.ii -o test.s   │
// └────────────┬────────────────────────────────────────────────┘
//              │
//              ▼ 汇编 (Assembly)
// ┌─────────────────────────────────────────────────────────────┐
// │ 目标文件 (.o)                                                │
// │                                                             │
// │   gcc -c test.s -o test.o        gcc -c test.s -o test.o    │
// │   clang -c test.s -o test.o    clang -c test.s -o test.o    │
// └────────────┬────────────────────────────────────────────────┘
//              │
//              ▼ 链接 (Linking)
// ┌─────────────────────────────────────────────────────────────┐
// │ 可执行程序 (Executable)                                      │
// │                                                             │
// │   gcc test.o -o test             gcc test.o -o test         │
// │   clang test.o -o test           clang test.o -o test       │
// └─────────────────────────────────────────────────────────────┘

// 词法分析 (Lexical Analysis)
//   clang -fmodules -fsyntax-only -Xclang -dump-tokens test_compile.cpp
//
// 语法分析 (Syntax Analysis)
//   clang -fmodules -fsyntax-only -Xclang -ast-dump test_compile.cpp
//
// 公共选项:
//   -fmodules      启用 Clang 模块特性
//   -fsyntax-only  仅做语法检查，不生成输出文件
//   -Xclang        将后续选项传递给 clang 前端（而非驱动）
//   -dump-tokens   输出词法分析的 token 列表
//   -ast-dump      输出语法分析的 AST（抽象语法树）

// 中间代码 (LLVM IR)
//   clang -emit-llvm -S test_compile.cpp -o test_compile.ll
//
// 选项:
//   -emit-llvm     生成 LLVM IR 而非本地机器码
//   -S             输出文本格式（配合 -emit-llvm 生成 .ll 文件）

#include <stdio.h>

#define N 1024

int main() {
    int a, b, c, d[N];
    a = 2;
    b = 4;
    c = a + b * 3;
    printf("Hello!");
    return c;
}
