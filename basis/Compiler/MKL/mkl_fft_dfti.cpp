// Intel MKL FFT 示例：一维复数 FFT（DFTI 接口）
//
// 什么是 MKL DFTI？
//   DFTI（Discrete Fourier Transform Interface）是 MKL 提供的离散傅里叶变换接口，
//   内部是针对 Intel 处理器高度优化的 FFT 实现（分治 + SIMD + 多线程），
//   远快于朴素的 O(N^2) 直接 DFT。
//
// 典型用法遵循「三步曲」：
//   1. DftiCreateDescriptor — 创建一个变换描述符（指定精度/域/维度/长度）
//   2. DftiCommitDescriptor — 提交（预计算 twiddle 因子、选择内核）
//   3. DftiCompute*         — 实际执行正/逆变换
//   最后 DftiFreeDescriptor 释放。
//   描述符可复用：commit 一次，能对多组数据反复 compute。
//
// 关键 API：
//   DftiCreateDescriptor(&desc, precision, domain, dim, length)
//     precision — DFTI_SINGLE / DFTI_DOUBLE
//     domain    — DFTI_COMPLEX / DFTI_REAL / DFTI_PACKED_FORMAT
//     dim       — 维度（1 表示一维）
//     length    — 该维度的变换长度
//   DftiSetValue(desc, config, value)     — 设置参数，如存储布局、原位/非原位
//     DFTI_BACKWARD_SCALE = 1/N           — 让正/逆变换还原（MKL 默认不正则化）
//     DFTI_PLACEMENT = DFTI_NOT_INPLACE   — 输入输出用不同缓冲区
//   DftiComputeForward(desc, x_in, x_out) — 正向变换
//   DftiComputeBackward(desc, x_in, x_out)— 反向变换
//
// 复数数组布局：MKL 默认用「交织」形式，re0,im0,re1,im1,...
//   因此 N 个复数需要长度为 2N 的 double/float 数组。
//
// 本例：对长度 N 的复数信号做正向 FFT，再做反向 FFT，验证能还原原始信号；
//   并顺手打印某个 bin 的幅度谱，演示如何解释结果。
//
// 编译命令（任选其一）：
//   clang++ -O2 mkl_fft_dfti.cpp -I$MKLROOT/include -L$MKLROOT/lib -lmkl_rt -lm -o mkl_fft_dfti
//   icpx -mkl mkl_fft_dfti.cpp -o mkl_fft_dfti

#include <mkl_dfti.h>  // Dfti* 接口
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define N 8  // 变换长度（演示用小尺寸，便于肉眼核对；实际可用 2 的幂如 1<<20）

// 检查 DFTI 返回码，非 0 即报错退出
static void check(MKL_LONG status, const char* what) {
    if (status != DFTI_NO_ERROR) {
        printf("DFTI error at %s: status = %ld\n", what, (long)status);
        exit(1);
    }
}

int main() {
    // 交织存储：2N 个 double 表示 N 个复数
    double* x = (double*)malloc(sizeof(double) * 2 * N);        // 原始信号
    double* X = (double*)malloc(sizeof(double) * 2 * N);        // 频域结果
    double* back = (double*)malloc(sizeof(double) * 2 * N);     // 逆变换还原

    // 构造一个简单信号：x[n] = n + i*(-n)，便于观察
    for (int n = 0; n < N; ++n) {
        x[2 * n] = n;        // real
        x[2 * n + 1] = -n;   // imag
    }

    DFTI_DESCRIPTOR_HANDLE fwd = NULL, inv = NULL;

    // --- 正向 FFT ---
    check(DftiCreateDescriptor(&fwd, DFTI_DOUBLE, DFTI_COMPLEX, 1, N), "create fwd");
    check(DftiSetValue(fwd, DFTI_PLACEMENT, DFTI_NOT_INPLACE), "set placement fwd");
    check(DftiCommitDescriptor(fwd), "commit fwd");
    check(DftiComputeForward(fwd, x, X), "compute fwd");

    // --- 反向 FFT（设 1/N 缩放，使 IFFT(FFT(x)) == x）---
    check(DftiCreateDescriptor(&inv, DFTI_DOUBLE, DFTI_COMPLEX, 1, N), "create inv");
    check(DftiSetValue(inv, DFTI_PLACEMENT, DFTI_NOT_INPLACE), "set placement inv");
    check(DftiSetValue(inv, DFTI_BACKWARD_SCALE, 1.0 / N), "set scale inv");
    check(DftiCommitDescriptor(inv), "commit inv");
    check(DftiComputeBackward(inv, X, back), "compute inv");

    // 打印频谱幅度 |X[k]|，并验证还原误差
    printf("MKL DFTI: 1-D complex FFT, N = %d\n", N);
    double max_err = 0.0;
    for (int k = 0; k < N; ++k) {
        double mag = sqrt(X[2 * k] * X[2 * k] + X[2 * k + 1] * X[2 * k + 1]);
        double er = fabs(back[2 * k] - x[2 * k]);  // 实部还原误差
        if (er > max_err) max_err = er;
        printf("  bin %d: |X| = %8.3f\n", k, mag);
    }
    printf("round-trip IFFT(FFT(x)) max abs error: %.2e\n", max_err);

    DftiFreeDescriptor(&fwd);
    DftiFreeDescriptor(&inv);
    free(x);
    free(X);
    free(back);
    return 0;
}
