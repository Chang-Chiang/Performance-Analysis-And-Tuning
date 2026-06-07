// 数学函数优化示例 (Math Function Optimization)
//
// 本例展示编译器对数学函数的优化。
//
// 编译器可以对数学函数进行以下优化:
//   1. 常量折叠 — 编译时计算 sin(30°) 的值，直接替换为常量
//   2. 内联展开 — 将 sin() 函数体展开，避免函数调用开销
//   3. 强度削减 — 用查表或近似算法替代精确计算
//   4. 融合运算 — 将 30 * PI / 180 合并为常量
//
// 优化前:
//   double a = (30 * PI / 180);
//   a = sin(a);
//
// 优化后（常量折叠）:
//   double a = 0.5;  // sin(30°) = 0.5
//
// 编译命令:
//   普通编译:
//     clang -O1 math_example.cpp -o math_example -lm
//
//   查看优化后的汇编:
//     clang -O1 -S math_example.cpp -o math_example.s
//
// 选项:
//   -O1  启用优化
//   -lm  链接数学库

#include <math.h>
#include <stdio.h>

#define PI 3.1415927

int main() {
    double a = (30 * PI / 180);  // 编译时可折叠为常量 0.523599
    a = sin(a);                  // 编译时可计算为 0.5
    printf("%lf\n", a);
    return 0;
}
