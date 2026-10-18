// Intel MKL VML 示例：向量数学函数库（Vector Math Library）
//
// 什么是 MKL VML？
//   VML（Vector Math Library）是 MKL 提供的「数组版」初等函数库：
//   一次调用对整个数组求 sin / cos / exp / ln / pow …，内部用 SIMD 打包，
//   相比在标量循环里逐个调用 <math.h> 的函数，能一次算多个元素。
//
// 命名规则：v + 精度字母 + 函数名
//   vsLn  — 单精度 float 数组求自然对数
//   vdLn  — 双精度 double 数组求自然对数
//   vsSin / vdSin — 数组求 sin
//   前缀 v 表示 vector；s/d/c/z = float/double/complex-f/complex-d。
//
// 精度模式（accuracy）：
//   0      — 默认（高精度，结果与 libm 几乎一致）
//   VML_HA — 高精度
//   VML_LA — 低精度（更快，误差不超过约 2^-14）
//   用 vmlSetMode(模式) 全局切换（返回旧模式），低精度 = 用精度换吞吐。
//
// 本例对比三种做法计算 ln(x) 数组：
//   (1) 标量循环：逐个调用 <math.h> 的 log()
//   (2) MKL vdLn 默认精度（高精度，SIMD 打包）
//   (3) MKL vdLn 低精度模式（VML_LA）
// 并给出耗时、加速比与最大绝对误差。
//
// 编译命令（任选其一）：
//   clang++ -O2 mkl_vml_log.cpp -I$MKLROOT/include -L$MKLROOT/lib -lmkl_rt -lm -o mkl_vml_log
//   icpx -O2 -mkl mkl_vml_log.cpp -o mkl_vml_log
//
// 选项：
//   -lmkl_rt  链接 MKL 运行时库（VML 包含在内）
//   -lm       链接 libm，供参考用的标量 log() 使用
//
// 关键点：
//   - 本例锁单线程（mkl_set_num_threads(1)），比的是「VML 的 SIMD 打包」而非并行，
//     结果稳定可复现；多线程需实测（Intel 大小核混合架构下 MKL 盲信核数可能变慢）。
//   - 经验提醒：并非所有 VML 函数都能稳赢 libm——ln/sqr 这类 SIMD 收益明显，
//     而个别函数（如某些机器上的 exp/sin）glibc 已极快，单线程下 MKL 未必占优。
//     「示例=用法演示」，性能务必以本机实测为准（见测量纪律手册）。

#include <math.h>
#include <mkl_service.h>        // mkl_set_num_threads
#include <mkl_vml_defines.h>    // VML_LA / VML_HA 模式常量
#include <mkl_vml_functions.h>  // vdLn / vmlSetMode
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define LEN 20000000  // 2000 万个元素

int main() {
    mkl_set_num_threads(1);  // 单线程：只比 SIMD 打包质量，剥离并行干扰

    double* x = (double*)malloc(sizeof(double) * LEN);
    double* y = (double*)malloc(sizeof(double) * LEN);
    double* ref = (double*)malloc(sizeof(double) * LEN);

    // 定义域取 [1, 1000]，均为正数，ln 良定义
    for (int i = 0; i < LEN; ++i)
        x[i] = 1.0 + (i % 1000);

    volatile double sink = 0.0;  // 防止循环被当死代码删除

    // --- (1) 标量循环：逐个调用 log()，同时留一份作为精度参考 ---
    clock_t t0 = clock();
    for (int i = 0; i < LEN; ++i) {
        y[i] = log(x[i]);
        ref[i] = y[i];
    }
    double t_scalar = (double)(clock() - t0) / CLOCKS_PER_SEC;
    sink = y[0];

    // --- (2) MKL VML 默认精度 ---
    clock_t t1 = clock();
    vdLn(LEN, x, y);
    double t_vml = (double)(clock() - t1) / CLOCKS_PER_SEC;
    sink = y[0];
    double err_ha = 0.0;
    for (int i = 0; i < LEN; ++i) {
        double e = fabs(y[i] - ref[i]);
        if (e > err_ha) err_ha = e;
    }

    // --- (3) MKL VML 低精度模式（VML_LA）：更快，误差略增 ---
    unsigned int old_mode = vmlSetMode(VML_LA);
    clock_t t2 = clock();
    vdLn(LEN, x, y);
    double t_vml_la = (double)(clock() - t2) / CLOCKS_PER_SEC;
    sink = y[0];
    vmlSetMode(old_mode);  // 复位为原来的精度模式
    double err_la = 0.0;
    for (int i = 0; i < LEN; ++i) {
        double e = fabs(y[i] - ref[i]);
        if (e > err_la) err_la = e;
    }

    printf("VML Vector Math (ln), N = %d, single thread\n", LEN);
    printf("scalar loop log()   : %.4f s\n", t_scalar);
    printf("vdLn  (default)     : %.4f s   speedup=%.2fx   max_err=%.2e\n", t_vml,
           t_scalar / t_vml, err_ha);
    printf("vdLn  (VML_LA)      : %.4f s   speedup=%.2fx   max_err=%.2e\n", t_vml_la,
           t_scalar / t_vml_la, err_la);

    free(x);
    free(y);
    free(ref);
    return 0;
}
