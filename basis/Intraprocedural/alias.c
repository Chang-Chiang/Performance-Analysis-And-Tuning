// 指针别名问题 (Pointer Aliasing Problem)
//
// 什么是指针别名？
//   当两个或多个指针引用相同的内存位置时，就存在指针别名。
//   编译器无法确定指针是否指向同一内存，因此无法进行某些优化。
//
// 指针别名的影响：
//   1. 优化困难 — 编译器不敢重排序或向量化涉及指针的操作
//   2. 性能下降 — 无法充分利用 CPU 流水线和 SIMD 指令
//   3. 未定义行为 — 别名指针的修改可能导致意外结果
//
// 解决方案：
//   C 语言：使用 restrict 关键字告诉编译器指针没有别名
//   C++ 语言：使用引用、const、移动语义等特性减少别名
//
// restrict 关键字：
//   限定指针变量，表示该指针是访问其所指向内存的唯一方式。
//   如果违反此约束，程序行为未定义。
//
// 编译命令：
//   gcc -O2 alias.c -o alias

#include <stdio.h>

#define N 1024

// 普通指针：编译器假设 a 和 b 可能有别名，无法优化
void add(int* a, int* b) {
    int C = 5;
    for (int i = 0; i < N; i++) {
        a[i] = b[i - 1] + C;
    }
}

// restrict 指针：编译器知道 a 和 b 没有别名，可以优化
void add_restrict(int* restrict a, int* restrict b) {
    int C = 5;
    for (int i = 0; i < N; i++) {
        a[i] = b[i - 1] + C;
    }
}

int main() {
    int a[N], b[N];

    for (int i = 0; i < N; i++) {
        a[i] = i;
        b[i] = i + 1;
    }

    add(a, b);
    printf("add(a, b): a[1] = %d\n", a[1]);

    add_restrict(a, b);
    printf("add_restrict(a, b): a[1] = %d\n", a[1]);

    // 注意：如果参数互为别名，使用 restrict 会导致错误结果
    // add_restrict(a, a);  // 未定义行为！

    return 0;
}
