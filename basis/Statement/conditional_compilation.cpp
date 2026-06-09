// 条件编译优化 (Conditional Compilation Optimization)
//
// 什么是条件编译优化？
//   使用预处理指令（如 #ifdef）在编译时决定代码路径，
//   而不是在运行时通过分支判断选择代码路径。
//
// 条件编译优化的作用：
//   1. 消除运行时分支 — 编译时确定代码路径，运行时无需判断
//   2. 减少代码体积 — 未使用的代码不会被编译
//   3. 提高执行效率 — 无分支代码更容易流水线化
//
// 示例分析：
//   优化前（运行时条件分支）：
//     switch (mode) {
//         case ON_ARM: arm_f(); break;
//         case ON_X86: x86_f(); break;
//     }
//     // 需要运行时判断，有分支跳转
//
//   优化后（条件编译）：
//     #ifdef ON_ARM
//         arm_f();
//     #elif ON_X86
//         x86_f();
//     #endif
//     // 编译时确定代码路径，无运行时分支
//
// 编译命令：
//   g++ -O2 conditional_compilation.cpp -o conditional_compilation

#include <stdio.h>

#define ON_ARM 1
#define ON_X86 2

void arm_f() { printf("ARM processor\n"); }
void x86_f() { printf("x86 processor\n"); }

// 示例 1：运行时条件分支
void example_runtime() {
    int mode = ON_ARM;

    printf("Runtime branch: ");
    switch (mode) {
        case ON_ARM:
            arm_f();
            break;
        case ON_X86:
            x86_f();
            break;
    }
}

// 示例 2：条件编译
void example_compile_time() {
    printf("Compile-time branch: ");
#ifdef ON_ARM
    arm_f();
#elif ON_X86
    x86_f();
#endif
}

int main() {
    example_runtime();
    example_compile_time();
    return 0;
}
