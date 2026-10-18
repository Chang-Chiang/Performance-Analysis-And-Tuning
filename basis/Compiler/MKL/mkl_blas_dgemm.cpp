// Intel MKL BLAS 示例：矩阵乘法 dgemm（朴素三重循环 vs MKL）
//
// 什么是 Intel MKL 的 BLAS 接口？
//   BLAS（Basic Linear Algebra Subprograms）是线性代数运算的标准接口，
//   分三个层次：
//     Level 1 — 向量-向量（dot / axpy / asum …）
//     Level 2 — 矩阵-向量（gemv / symv …）
//     Level 3 — 矩阵-矩阵（gemm / symm …）  ← 计算密集、最容易吃满算力
//   Intel MKL 提供了 BLAS 的高性能实现，并用 C 接口（cblas_*）暴露出来。
//   针对 Intel 处理器，MKL 会用到分块 + SIMD + FMA + 多线程，
//   因此「直接调 cblas_dgemm 通常远快于自己写三重循环」。
//
// 命名规则：dgemm = d(ouble 双精度) + ge(neral 普通矩阵) + m(matrix) m(multiply)
//   前缀 s/d/c/z 分别对应 float / double / complex<float> / complex<double>。
//
// 本例用 cblas_dgemm 计算 C = alpha * A * B + beta * C：
//   cblas_dgemm(order, transa, transb, M, N, K, alpha, A, lda,
//               B, ldb, beta, C, ldc)
//     order  — 存储顺序（CblasRowMajor 行优先 / CblasColMajor 列优先）
//     transa — A 是否转置（CblasNoTrans / CblasTrans / CblasConjTrans）
//     transb — B 是否转置
//     M      — C 的行数（A 的行数，若 A 转置则为列数）
//     N      — C 的列数（B 的列数，若 B 转置则为行数）
//     K      — A 的列数 = B 的行数（内积维度）
//     alpha  — 标量系数
//     A      — 矩阵 A 数据指针
//     lda    — A 的 leading dimension（行优先时 = A 的列数）
//     B      — 矩阵 B 数据指针
//     ldb    — B 的 leading dimension
//     beta   — 标量系数（为 0 时 C 不必预先有值）
//     C      — 结果矩阵（同时是输入，用于 beta*C 累加）
//     ldc    — C 的 leading dimension
//
// 编译命令（任选其一）：
//   clang++ -O2 mkl_blas_dgemm.cpp -I$MKLROOT/include -L$MKLROOT/lib -lmkl_rt -o mkl_blas_dgemm
//   icpx -mkl mkl_blas_dgemm.cpp -o mkl_blas_dgemm        # Intel 编译器一键链接 MKL
//
// 运行前建议：
//   source /opt/intel/oneapi/setvars.sh   # 设置 MKLROOT 及动态库搜索路径
//
// 关键点：
//   - 本例把 MKL 锁为单线程（mkl_set_num_threads(1)），做的是
//     「MKL 打包内核 vs 手写三重循环」的公平对比——剥离并行因素，只比内核质量
//     （分块 + SIMD + FMA）。要试多线程，改成 mkl_set_num_threads(n) 或设 MKL_NUM_THREADS。
//   - ⚠️ Intel 大小核混合架构上，MKL 盲信核数反而可能因超线程/频率不均而变慢，
//     多线程扩展性需实测确认（见 10 号笔记 / 测量纪律手册）。
//   - 计时用 volatile sink 防止编译器把整段循环当死代码删除。

#include <mkl_cblas.h>   // cblas_dgemm 等 BLAS C 接口
#include <mkl_service.h> // mkl_set_num_threads / mkl_get_version
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define N 1000  // 方阵边长，N^3 次乘加，够大到压出差距又不难等待

// 朴素三重循环矩阵乘法：C = A * B（行优先存储）
// 循环顺序选 i-k-j，让内层对 B、C 的访问都是连续内存，是「手写能达到的较好形状」，
// 但依然远不及 MKL 的分块 + 向量化内核。
static void naive_gemm(int n, const double* A, const double* B, double* C) {
    for (int i = 0; i < n; ++i)
        for (int k = 0; k < n; ++k) {
            double a_ik = A[i * n + k];
            for (int j = 0; j < n; ++j)
                C[i * n + j] += a_ik * B[k * n + j];
        }
}

static void fill(int n, double* M) {
    for (int i = 0; i < n; ++i)
        for (int j = 0; j < n; ++j)
            M[i * n + j] = (i + j + 1) % 10;  // 简单可复现的确定性数据
}

int main() {
    mkl_set_num_threads(1);  // 单线程：只比内核质量，剥离并行干扰，结果可复现
    char ver[256];
    mkl_get_version_string(ver, sizeof(ver));  // 打印 MKL 版本，便于复现
    printf("MKL version: %s\n", ver);
    printf("MKL threads: %d\n\n", mkl_get_max_threads());

    double *A = (double*)malloc(sizeof(double) * N * N);
    double *B = (double*)malloc(sizeof(double) * N * N);
    double *C = (double*)malloc(sizeof(double) * N * N);
    fill(N, A);
    fill(N, B);

    // 乘加次数 = 2 * N^3，用于换算 GFLOPS
    double flops = 2.0 * (double)N * (double)N * (double)N;

    // --- 朴素三重循环 ---
    for (int i = 0; i < N * N; ++i) C[i] = 0.0;
    clock_t t0 = clock();
    naive_gemm(N, A, B, C);
    double t_naive = (double)(clock() - t0) / CLOCKS_PER_SEC;
    volatile double sink = C[0];  // 防止结果被优化掉

    // --- MKL cblas_dgemm ---
    for (int i = 0; i < N * N; ++i) C[i] = 0.0;
    double alpha = 1.0, beta = 0.0;
    clock_t t1 = clock();
    cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, N, N, N, alpha, A, N, B, N, beta,
               C, N);
    double t_mkl = (double)(clock() - t1) / CLOCKS_PER_SEC;
    sink = C[0];

    printf("Matrix size      : %d x %d\n", N, N);
    printf("naive_gemm       : %.4f s   (%.2f GFLOPS)\n", t_naive, flops / t_naive / 1e9);
    printf("cblas_dgemm (MKL): %.4f s   (%.2f GFLOPS)\n", t_mkl, flops / t_mkl / 1e9);
    printf("speedup          : %.1fx\n", t_naive / t_mkl);

    free(A);
    free(B);
    free(C);
    return 0;
}
