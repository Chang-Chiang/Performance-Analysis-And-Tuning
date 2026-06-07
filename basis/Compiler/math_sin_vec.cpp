// 向量化正弦函数示例 (Vectorized Sin Function)
//
// 本例展示如何通过手动向量化提升数学函数的计算性能。
//
// 优化思路:
//   标量 sin() 函数每次只能计算一个值，而 SIMD 指令可以同时计算多个值。
//   通过将多个 sin() 调用打包，可以利用 SIMD 并行计算。
//
// 优化前（标量）:
//   for (j = 0; j < N; j++) {
//       result = sin(pi / 2);  // 每次计算 1 个值
//   }
//
// 优化后（向量化）:
//   for (j = 0; j < N; j += 4) {
//       result = Sin(x1, x2, x3, x4);  // 每次计算 4 个值
//   }
//
// 编译命令:
//   clang++ -O1 sin_vec.cpp -o sin_vec -lm
//
// 选项:
//   -O1  启用优化
//   -lm  链接数学库

#include <math.h>
#include <stdio.h>
#include <time.h>
#include <x86intrin.h>

#define PI 3.1415926

// 自定义向量化 Sin 函数（示例，实际应使用 SIMD 指令）
double Sin(double x1, double x2, double x3, double x4) {
    double result = 0.0;

    result = sin(x1);
    result = sin(x2);
    result = sin(x3);
    result = sin(x4);

    return result;
}

int main() {
    clock_t start, end;
    // double  result1;  // 编译器会把循环优化掉，因为 result1 每次被覆盖，只保留最后一次结果
    volatile double result1 = 0; // volatile 阻止编译器优化
    double          time1   = 0;

    // 测试自定义 Sin 函数
    start = clock();
    for (int j = 1; j <= 100000000; j += 4) {
        result1 = Sin(PI / 2, PI / 2, PI / 2, PI / 2);
    }
    end   = clock();
    time1 = end - start;
    printf("自定义Sin函数计算sin(pi/2)=%f 用时 %f s\n", result1, (double)time1 / CLOCKS_PER_SEC);

    // 测试原始 sin 函数
    start = clock();
    for (int j = 1; j <= 100000000; j++) {
        result1 = sin(PI / 2);
    }
    end   = clock();
    time1 = end - start;
    printf("原始Sin函数计算sin(pi/2)=%f 用时 %f s\n", result1, (double)time1 / CLOCKS_PER_SEC);

    return 0;
}
