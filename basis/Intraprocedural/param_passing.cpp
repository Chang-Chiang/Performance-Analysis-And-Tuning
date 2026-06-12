// 函数参数传递优化 (Function Parameter Passing Optimization)
//
// 函数调用时参数传递方式：
//   1. 寄存器传递 — 优先使用寄存器，读写只需 1 个时钟周期
//   2. 栈传递 — 超出寄存器数量后使用栈，可能需要数十个时钟周期
//
// 参数传递的开销：
//   参数越多，开销越大。超出寄存器数量后，参数通过栈传递，性能下降。
//
// 优化策略：
//   将多个参数组合成一个结构体，传递结构体指针。
//   这样只需传递一个指针（8 字节），而不是多个参数。
//
// 优化前：
//   void func(int x, int y, int z, int a, int b, int c)  // 6 个参数
//
// 优化后：
//   struct Param { int x, y, z, a, b, c; };
//   void func(struct Param* p)  // 1 个指针
//
// 编译命令：
//   g++ -O2 param_passing.cpp -o param_passing

#include <stdio.h>

// 参数结构体
struct Param {
    int x;
    int y;
    int z;
    int a;
    int b;
    int c;
};

// 优化前：传递 6 个参数
void func(int x, int y, int z, int a, int b, int c) {
    x = a + b;
    y = b + c;
    z = a + c;
}

// 优化后：传递 1 个结构体指针
void func_param(struct Param* p) {
    p->x = p->a + p->b;
    p->y = p->b + p->c;
    p->z = p->a + p->c;
}

int main() {
    int a = 1, b = 2, c = 3;

    // 优化前：传递 6 个参数
    func(0, 0, 0, a, b, c);
    printf("传递 6 个参数：开销较大\n");

    // 优化后：传递 1 个结构体指针
    struct Param p;
    p.a = 1;
    p.b = 2;
    p.c = 3;
    func_param(&p);
    printf("传递结构体指针：开销较小\n");

    return 0;
}
