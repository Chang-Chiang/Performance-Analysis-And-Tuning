// Pragma 向量化参数控制 (Pragma Vectorization Parameters)
//
// 本例展示如何通过 pragma 指定向量化宽度和交错次数。
//
// #pragma clang loop vectorize_width(4)   — 向量化宽度为 4（每次处理 4 个元素）
// #pragma clang loop interleave_count(8)  — 交错次数为 8（同时执行 8 个向量操作）
//
// 向量化宽度 (Vectorization Width):
//   每条向量指令处理的数据元素个数。
//   例如：vectorize_width(4) 表示用 128 位 SIMD 寄存器处理 4 个 int
//
// 交错次数 (Interleave Count):
//   每次循环迭代中同时执行的向量操作个数。
//   例如：interleave_count(8) 表示每次迭代执行 8 个向量操作
//   总共每次迭代处理 4 × 8 = 32 个元素
//
// #pragma clang optimize off — 禁用后续代码的所有优化
//
// 编译命令:
//   clang -O1 pragma_vectorization_param.cpp -o pragma_vectorization_param
//
// 选项:
//   -O1  启用优化（pragma 需要优化支持）

#include <stdio.h>
#include <stdlib.h>

int main() {
    int N = 1024;
    int sum = 0;
    int A[N], B[N];

    for (int i = 0; i < N; ++i) {
        A[i] = i;
        B[i] = rand() % 10;
    }

    // 向量化宽度 4，交错次数 8，每次迭代处理 4 × 8 = 32 个元素
    #pragma clang loop vectorize_width(4) interleave_count(8)
    for (int i = 0; i < N; ++i) {
        sum = sum + A[i] + B[i];
    }

    printf("%d\n", sum);

    #pragma clang optimize off
    return 0;
}
