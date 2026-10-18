// Intel MKL LAPACK 示例：解线性方程组 dgesv（LAPACKE C 接口）
//
// 什么是 MKL LAPACK？
//   LAPACK（Linear Algebra PACKage）build 在 BLAS 之上，提供矩阵分解、
//   特征值、SVD、线性方程组求解等高阶数值算法。MKL 内含 LAPACK 的
//   高度优化实现，并提供两种调用方式：
//     (1) 传统 Fortran 接口：dgesv_（列优先，参数按位置传）
//     (2) LAPACKE C 接口：LAPACKE_dgesv（可指定行/列优先，更符合 C 习惯）
//   本例用 (2) 的 LAPACKE_dgesv，直接以行优先（LAPACK_ROW_MAJOR）求解。
//
// dgesv = solve general system（普通稠密方阵 Ax = b）
//   内部走 LU 分解（getrf）+ 回代（getrs），并做部分主元选取（pivot）。
//   前缀 s/d/c/z = float/double/complex-f/complex-d。
//
//   LAPACKE_dgesv(layout, n, nrhs, A, lda, ipiv, B, ldb)
//     layout — LAPACK_ROW_MAJOR / LAPACK_COL_MAJOR
//     n      — 方程个数 = 矩阵阶数
//     nrhs   — 右端项列数（解多个向量时 >1）
//     A      — 系数矩阵（输入），成功时返回 LU 因子
//     lda    — A 的 leading dimension
//     ipiv   — 主元索引数组（长度 n，内部使用）
//     B      — 右端项（输入），成功时原地被解 X 覆盖
//     ldb    — B 的 leading dimension
//   返回 0 = 成功；<0 参数非法；>0 表示 U 的对角元为 0（矩阵奇异）。
//
// 编译命令（任选其一）：
//   clang++ -O2 mkl_lapack_dgesv.cpp -I$MKLROOT/include -L$MKLROOT/lib -lmkl_rt -o mkl_lapack_dgesv
//   icpx -mkl mkl_lapack_dgesv.cpp -o mkl_lapack_dgesv
//
// 关键点：
//   - 用「已知真解 x_true」反推右端 b = A*x_true，再让 LAPACK 求解，
//     可同时验证正确性并打印残差 ||A x - b||，贴合工程的「测量纪律」。

#include <mkl_lapacke.h>  // LAPACKE_* C 接口
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define N 5

int main() {
    double* A = (double*)malloc(sizeof(double) * N * N);
    double* B = (double*)malloc(sizeof(double) * N);
    double* b_orig = (double*)malloc(sizeof(double) * N);
    int     ipiv[N];

    // 构造一个对角占优的确定性矩阵 A，保证可解、数值稳定
    for (int i = 0; i < N; ++i)
        for (int j = 0; j < N; ++j)
            A[i * N + j] = (i == j) ? (3.0 * i + 10.0) : (1.0 / (i + j + 2.0));

    // 已知真解 x_true = [1, 2, ..., N]，反推右端项 b = A * x_true
    double x_true[N];
    for (int k = 0; k < N; ++k) x_true[k] = k + 1.0;
    for (int i = 0; i < N; ++i) {
        double s = 0.0;
        for (int j = 0; j < N; ++j) s += A[i * N + j] * x_true[j];
        B[i] = s;
    }

    // 保留原始 A 与 b，便于求解后计算残差 ||A*x - b||（LAPACK 会就地覆盖 A、B）
    double* Acopy = (double*)malloc(sizeof(double) * N * N);
    for (int i = 0; i < N * N; ++i) Acopy[i] = A[i];
    for (int i = 0; i < N; ++i) b_orig[i] = B[i];

    // --- 求解 A * X = B ---
    lapack_int info = LAPACKE_dgesv(LAPACK_ROW_MAJOR, N, 1, A, N, ipiv, B, 1);
    if (info != 0) {
        printf("LAPACKE_dgesv failed, info = %d\n", (int)info);
        return 1;
    }

    // B 现在就是解 X。计算相对误差与残差 ||A x - b||
    double max_rel_err = 0.0, resid = 0.0;
    for (int i = 0; i < N; ++i) {
        double e = fabs(B[i] - x_true[i]) / fabs(x_true[i]);
        if (e > max_rel_err) max_rel_err = e;
        double s = 0.0;
        for (int j = 0; j < N; ++j) s += Acopy[i * N + j] * B[j];  // A * x
        double r = s - b_orig[i];                                   // A x - b
        resid += r * r;
    }
    resid = sqrt(resid);

    printf("LAPACK dgesv: solving %dx%d linear system A x = b\n", N, N);
    for (int i = 0; i < N; ++i)
        printf("  x[%d] = %8.4f   (expected %6.1f)\n", i, B[i], x_true[i]);
    printf("max relative error vs x_true : %.2e\n", max_rel_err);
    printf("residual ||A x - b||         : %.2e\n", resid);

    free(A);
    free(B);
    free(b_orig);
    free(Acopy);
    return 0;
}
