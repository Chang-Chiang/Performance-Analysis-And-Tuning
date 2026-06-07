// CBLAS 矩阵向量乘法示例 (CBLAS Matrix-Vector Multiplication)
//
// 什么是 CBLAS？
//   CBLAS 是 BLAS（Basic Linear Algebra Subprograms）的 C 语言接口。
//   BLAS 是线性代数运算的标准库，提供向量和矩阵的基本运算。
//
// CBLAS 的作用:
//   1. 标准接口 — 跨平台、跨实现的线性代数运算标准
//   2. 高度优化 — 针对不同硬件高度优化
//   3. 广泛支持 — Intel MKL、OpenBLAS、ATLAS 等都实现 CBLAS 接口
//
// 本例使用 cblas_zgemv 计算复数矩阵向量乘法:
//   y = alpha * A * x + beta * y
//
//   cblas_zgemv(layout, trans, M, N, alpha, A, lda, x, incx, beta, y, incy)
//     layout — 矩阵存储顺序（CblasColMajor 列优先，CblasRowMajor 行优先）
//     trans  — 是否转置（CblasNoTrans 不转置，CblasTrans 转置）
//     M      — 矩阵行数
//     N      — 矩阵列数
//     alpha  — 标量系数
//     A      — 矩阵数据
//     lda    — 矩阵 leading dimension
//     x      — 输入向量
//     incx   — 向量 x 的步长
//     beta   — 标量系数
//     y      — 输出向量
//     incy   — 向量 y 的步长
//
// 编译命令:
//   clang++ math_cblas.cpp -I ./include/ blas_LINUX.a cblas_LINUX.a -lgfortran -o math_cblas
//
// 选项:
//   -I ./include/  — 指定 CBLAS 头文件路径
//   blas_LINUX.a   — 链接 BLAS 静态库
//   cblas_LINUX.a  — 链接 CBLAS 静态库
//   -lgfortran     — 链接 Fortran 运行时库（BLAS 底层用 Fortran 实现）

#include <cblas.h>

#include <complex>
#include <iostream>

int main() {
    using namespace std;
    typedef complex<double> Comp;

    int   Nr = 2, Nc = 3;
    Comp* a = new Comp[Nr * Nc];  // 2x3 复数矩阵
    Comp* x = new Comp[Nc];       // 输入向量（3 维）
    Comp* y = new Comp[Nr];       // 输出向量（2 维）
    Comp  alpha(1, 0), beta(0, 0);

    // 初始化输入向量 x
    for (int i = 0; i < Nc; ++i) {
        x[i] = Comp(i + 1., i + 2.);
    }

    // 初始化矩阵 a（列优先存储）
    for (int i = 0; i < Nr * Nc; ++i) {
        a[i] = Comp(i + 1., i + 2.);
    }

    // 计算 y = alpha * A * x + beta * y
    cblas_zgemv(CblasColMajor, CblasNoTrans, Nr, Nc, &alpha, a, Nr, x, 1, &beta, y, 1);

    // 打印输入向量 x
    for (int i = 0; i < Nc; ++i) {
        cout << x[i] << "  ";
    }
    cout << "\n" << endl;

    // 打印矩阵 a
    for (int i = 0; i < Nr; ++i) {
        for (int j = 0; j < Nc; ++j) {
            cout << a[i + Nr * j] << "  ";
        }
        cout << endl;
    }
    cout << "\n" << endl;

    // 打印输出向量 y
    for (int i = 0; i < Nr; ++i) {
        cout << y[i] << "  ";
    }

    delete[] a;
    delete[] x;
    delete[] y;
    return 0;
}
