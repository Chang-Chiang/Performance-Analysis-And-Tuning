// Intel MKL 数学库示例 (Intel Math Kernel Library)
//
// 什么是 Intel MKL？
//   Intel Math Kernel Library 是 Intel 提供的高性能数学库，
//   针对 Intel 处理器高度优化，提供 BLAS、LAPACK、FFT 等数学函数。
//
// MKL 的作用:
//   1. 高性能 — 针对 Intel 处理器优化，比标准数学库快数倍
//   2. 跨平台 — 支持 Windows、Linux、macOS
//   3. 接口标准 — 兼容 BLAS、LAPACK、FFT 等标准接口
//   4. 自动并行 — 支持多线程自动并行
//
// 本例使用 cblas_dzasum 计算复数向量的绝对值之和:
//   cblas_dzasum(N, vector, stride)
//     N      — 向量长度
//     vector — 指向复数向量的指针
//     stride — 步长（1 表示连续访问）
//
// 编译命令:
//   clang++ -O1 math_mkl.cpp -o math_mkl -lmkl_rt
//   icpx math_mkl.cpp -I$MKLROOT/include -L$MKLROOT/lib -lmkl_rt -o math_mkl
//
// icpx 命令解释:
//   icpx                  — Intel oneAPI C++ 编译器（基于 LLVM，替代 icc）
//   math_mkl.cpp          — 源文件
//   -I$MKLROOT/include    — 指定 MKL 头文件路径（$MKLROOT=/opt/intel/oneapi/mkl/2026.0）
//   -L$MKLROOT/lib        — 指定 MKL 库文件路径
//   -lmkl_rt              — 链接 MKL 运行时库
//   -o math_mkl           — 输出可执行文件名
//
// 环境变量设置:
//   source /opt/intel/oneapi/setvars.sh  — 设置 MKLROOT 等环境变量
//
// clang/gcc 命令解释:
//   clang++      — Clang C++ 编译器
//   -O1          — 启用优化
//   -lmkl_rt     — 链接 MKL 运行时库

#include <malloc.h>
#include <mkl_cblas.h>
#include <stdio.h>

#define N 10

void initVector(MKL_Complex16* v) {
    for (int i = 0; i < N; i++) {
        v[i].real = -i * 1.0f;
        v[i].imag = i * 1.0f;
    }
}

int main(int argc, char* argv[]) {
    MKL_Complex16* vector = (MKL_Complex16*)malloc(sizeof(MKL_Complex16) * N);
    initVector(vector);

    // 计算复数向量的绝对值之和
    double ret3 = cblas_dzasum(N, vector, 1);
    printf("Result of sasum:%lf\n", ret3);

    free(vector);
    return 0;
}
