// Pragma 循环分布控制 (Pragma Loop Distribution)
//
// 本例展示如何通过 pragma 指令控制循环分布。
//
// 常用 pragma 指令:
//   #pragma clang loop distribute(enable)   — 启用循环分布
//   #pragma clang loop distribute(disable)  — 禁用循环分布
//
// 本例中的循环包含两条语句:
//   A[i + 1] = A[i] + B[i];  // 有循环携带依赖（A[i+1] 依赖 A[i]）
//   C[i]     = D[i] * E[i];  // 无依赖，可向量化
//
// 循环分布后:
//   for (i = 0; i < N; ++i) { A[i + 1] = A[i] + B[i]; }  // 有依赖，标量执行
//   for (i = 0; i < N; ++i) { C[i]     = D[i] * E[i]; }  // 无依赖，可向量化
//
// 编译命令:
//   clang -O1 pragma_loop_distribute.cpp -o pragma_loop_distribute
//
// 选项:
//   -O1  启用优化（pragma 需要优化支持）

#include <stdio.h>

int main() {
    int N = 1024;
    int A[N], B[N], C[N], D[N], E[N];

    for (int i = 0; i < N; ++i) {
        A[i] = i;
        B[i] = i + 1;
        D[i] = i + 2;
        E[i] = i + 3;
    }

// 启用循环分布，将有依赖和无依赖的语句分开
#pragma clang loop distribute(enable)
    for (int i = 0; i < N; ++i) {
        A[i + 1] = A[i] + B[i]; // 有循环携带依赖
        C[i]     = D[i] * E[i]; // 无依赖，可向量化
    }

    printf("%d", A[8]);
    return 0;
}
