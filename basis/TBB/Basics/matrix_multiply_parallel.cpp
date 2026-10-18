/**
 * 矩阵乘法 - oneTBB 并行版本
 *
 * 原理：
 * 用 tbb::parallel_for 把最外层（行 i）并行化：每个任务处理一批行，
 * 行间无数据依赖 → 安全并行。与 OpenMP 版对应，但：
 * - 不需要 shared/private 子句（闭包捕获即可）；
 * - 线程数由运行时工作窃取调度器管理，可用 tbb::global_control 设定上限；
 * - 外层并行天然把 fork/join 开销摊到一次调用（见第 8 课"并行区扩张"）。
 *
 * 编译指令：
 * g++ -O2 -std=c++17 -march=native matrix_multiply_parallel.cpp -o matrix_multiply_parallel $(pkg-config --cflags --libs tbb)
 *
 * 运行：
 * ./matrix_multiply_parallel            # 用默认并发度（本机=24）
 * ./matrix_multiply_parallel 4          # 上限 4 个 worker
 * ./matrix_multiply_parallel 8          # 上限 8 个 worker
 */

#include <oneapi/tbb.h>
#include <oneapi/tbb/global_control.h>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <chrono>
#include <memory>

#ifndef N
#define N 1200
#endif

int main(int argc, char** argv) {
    int nthreads = argc > 1 ? atoi(argv[1]) : 0; // 0 = 用默认并发度

    std::vector<float> A(size_t(N) * N, 0.f), B(size_t(N) * N, 0.f), C(size_t(N) * N, 0.f);
    for (int i = 0; i < N; ++i)
        for (int j = 0; j < N; ++j) {
            A[size_t(i) * N + j] = i + 1.0f;
            B[size_t(i) * N + j] = 1.0f;
        }

    // global_control 是一个 RAII 作用域：在其存活期间限制 TBB 允许的最大并行度。
    // 用来做"线程数扫描"找加速比拐点（呼应第 8 课实验⑦）。
    std::unique_ptr<tbb::global_control> gc;
    if (nthreads > 0)
        gc = std::make_unique<tbb::global_control>(
            tbb::global_control::max_allowed_parallelism, nthreads);

    int effective = tbb::info::default_concurrency();

    auto t0 = std::chrono::high_resolution_clock::now();

    // 只并行外层 i：迭代数 = N，足够多；内层保持 i-k-j 的连续访存
    tbb::parallel_for(
        tbb::blocked_range<int>(0, N, /*grainsize=*/64),
        [&](const tbb::blocked_range<int>& rows) {
            for (int i = rows.begin(); i != rows.end(); ++i) {
                float* crow = &C[size_t(i) * N];
                const float* arow = &A[size_t(i) * N];
                for (int k = 0; k < N; ++k) {
                    float a = arow[k];
                    const float* brow = &B[size_t(k) * N];
                    for (int j = 0; j < N; ++j)
                        crow[j] += a * brow[j];
                }
            }
        });

    auto t1 = std::chrono::high_resolution_clock::now();
    double sec = std::chrono::duration<double>(t1 - t0).count();

    double sum = 0.0;
    for (size_t idx = 0; idx < size_t(N) * N; ++idx) sum += C[idx];
    printf("TBB并行版 N=%d  workers<=%d  concurrency=%d  sum=%.0f  time=%.4f s\n",
           N, nthreads > 0 ? nthreads : effective, effective, sum, sec);
    return 0;
}

/**
 * 并行化分析（对照第 8 课 OpenMP 版）：
 *
 * 1. 切分：外层 i 天然独立，blocked_range 按行对分；grainsize=64 给一个下限，
 *    调度器再工作窃取地细分——等价于 OpenMP 的 static 但会自适应。
 * 2. 合并：C 各段互不相交，无需归约，也就没有伪共享之外的同步成本。
 * 3. 均不均：行计算量一致（稠密阵），工作窃取下负载自然均衡。
 *
 * 预期：N=1200 稠密浮点乘是计算密集 + 连续访存，前段近线性；
 *      到物理核数后受 Amdahl + 内存带宽封顶（本机大约 12x 量级，见笔记实测）。
 */
