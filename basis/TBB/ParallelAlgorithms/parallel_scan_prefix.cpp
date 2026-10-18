/**
 * parallel_scan 并行前缀和（prefix sum / scan）
 *
 * 原理：
 * 前缀和是"每个输出依赖其之前所有输入"的典型串行依赖，朴素 parallel_for 无法直接并行。
 * TBB 的 parallel_scan 用两趟递归对分策略：
 * - 完整扫描（final pass）：算出每段区间对结果的贡献并写回；
 * - 部分扫描（prepass）：只为上层合并准备"这段的局部和"。
 * 关键：同一份 scan 代码用 `final` 标志区分两种阶段——只有 final 才写输出。
 *
 * 应用：直方图均衡、流压缩（stream compaction）、并行分配器 offset、稀疏矩阵行偏移等。
 *
 * 编译指令：
 * g++ -O2 -std=c++17 parallel_scan_prefix.cpp -o parallel_scan_prefix $(pkg-config --cflags --libs tbb)
 *
 * 运行：
 * ./parallel_scan_prefix
 */

#include <oneapi/tbb.h>
#include <cstdio>
#include <vector>

#define N 1000000

int main() {
    std::vector<int> in(N, 1), excl(N, 0), incl(N, 0);
    for (int i = 0; i < N; ++i) in[i] = i % 2 ? 2 : 1; // 制造一些不同值

    // 串行参照
    long run = 0;
    std::vector<int> sExcl(N), sIncl(N);
    for (int i = 0; i < N; ++i) { sExcl[i] = run; run += in[i]; sIncl[i] = run; }

    // ---------- 排他前缀和（exclusive）：out[i] = sum(in[0..i-1]) ----------
    // scan lambda：给定区间 + 进入该区间前的累计和 sum + final 标志，返回区间结束时的和
    //   final=true 时才把结果写回 out（prepass 阶段只算不写）
    // reverse_join lambda：合并两个子区间的和，必须返回合并值
    tbb::parallel_scan(
        tbb::blocked_range<int>(0, N, 10000), /*init=*/0,
        [&](const tbb::blocked_range<int>& r, int sum, bool final) -> int {
            for (int i = r.begin(); i != r.end(); ++i) {
                if (final) excl[i] = sum;   // 排他：写"加上 a[i] 之前"的累计值
                sum += in[i];
            }
            return sum;
        },
        [](int first, int second) { return first + second; }); // reverse_join：返回合并后的和

    // ---------- 包含前缀和（inclusive）：out[i] = sum(in[0..i]) ----------
    tbb::parallel_scan(
        tbb::blocked_range<int>(0, N, 10000), /*init=*/0,
        [&](const tbb::blocked_range<int>& r, int sum, bool final) -> int {
            for (int i = r.begin(); i != r.end(); ++i) {
                sum += in[i];
                if (final) incl[i] = sum;   // 包含：写"加上 a[i] 之后"的累计值
            }
            return sum;
        },
        [](int first, int second) { return first + second; });

    // 校验
    bool okExcl = true, okIncl = true;
    for (int i = 0; i < N; ++i) {
        if (excl[i] != sExcl[i]) okExcl = false;
        if (incl[i] != sIncl[i]) okIncl = false;
    }
    printf("排他前缀和 正确=%s  excl[N-1]=%d (串行 %d)\n", okExcl ? "YES" : "NO", excl[N - 1], sExcl[N - 1]);
    printf("包含前缀和 正确=%s  incl[N-1]=%d (串行 %d)\n", okIncl ? "YES" : "NO", incl[N - 1], sIncl[N - 1]);
    return 0;
}

/**
 * parallel_scan 的两个易错点：
 * 1. scan lambda 里 `if (final)` 才写输出——prepass 阶段同一函数会被以 final=false
 *    调用去算局部和，若无条件写会读到还没定稿的前缀 → 结果错。
 * 2. reverse_join（合并函数）在 oneTBB 的 lambda 版里【必须返回合并值】
 *    `Value(const Value&, const Value&)`，写成 void 会编译失败（老 TBB 是就地修改）。
 *
 * 对照 OpenMP：OpenMP 5.x 有 `#pragma omp scan`；MPI 有 MPI_Scan。三者都是同一种
 * "分段 + 前向合并"的抽象，这正是第 21 课说的跨模型母题。
 */
