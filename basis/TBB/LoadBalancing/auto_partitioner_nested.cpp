/**
 * auto_partitioner + 嵌套并行：递归对分如何优雅处理"并行里再并行"
 *
 * 原理：
 * auto_partitioner（不写分区器时的默认）在运行时【递归对分 + 工作窃取】。
 * 它的一大价值是能安全处理【嵌套并行】：外层任务执行中又派生内层并行，
 * 调度器把它们统一塞进同一张任务图，空闲线程继续窃取，而不是像朴素
 * fork-join 那样"线程数指数爆炸"。
 *
 * 本例：二维网格，外层按行块并行；某些行块再触发内层 parallel_reduce 求该行块和。
 * 对比"关闭内层并行"看嵌套是否真带来收益/是否安全。
 *
 * 编译指令：
 * g++ -O2 -std=c++17 auto_partitioner_nested.cpp -o auto_partitioner_nested $(pkg-config --cflags --libs tbb)
 *
 * 运行：
 * ./auto_partitioner_nested
 */

#include <oneapi/tbb.h>
#include <cstdio>
#include <chrono>
#include <vector>

static const int ROWS = 4000;
static const int COLS = 4000;

int main() {
    std::vector<double> M(size_t(ROWS) * COLS);
    for (size_t i = 0; i < M.size(); ++i) M[i] = i % 997;

    auto t0 = std::chrono::high_resolution_clock::now();
    // 外层：按行块并行；每个行块内部再做一次 parallel_reduce（嵌套并行）
    double grand = tbb::parallel_reduce(
        tbb::blocked_range<int>(0, ROWS, 250), 0.0,
        [&](const tbb::blocked_range<int>& rows, double base) {
            // 内层：对本行块再并行求和 —— auto 让内外层任务共用线程池，窃取到满
            double local = tbb::parallel_reduce(
                tbb::blocked_range<int>(rows.begin(), rows.end()), 0.0,
                [&](const tbb::blocked_range<int>& rr, double s) {
                    for (int r = rr.begin(); r != rr.end(); ++r)
                        for (int c = 0; c < COLS; ++c) s += M[size_t(r) * COLS + c];
                    return s;
                },
                [](double a, double b) { return a + b; });
            return base + local;
        },
        [](double a, double b) { return a + b; });
    auto t1 = std::chrono::high_resolution_clock::now();

    // 纯平铺（不嵌套）对照：一次性 parallel_reduce 扫全表
    auto t2 = std::chrono::high_resolution_clock::now();
    double flat = tbb::parallel_reduce(
        tbb::blocked_range<size_t>(0, M.size(), 100000), 0.0,
        [&](const tbb::blocked_range<size_t>& r, double s) {
            for (size_t i = r.begin(); i != r.end(); ++i) s += M[i];
            return s;
        },
        [](double a, double b) { return a + b; });
    auto t3 = std::chrono::high_resolution_clock::now();

    printf("嵌套并行   : %.3fs  (grand=%.0f)\n", std::chrono::duration<double>(t1 - t0).count(), grand);
    printf("平铺单层   : %.3fs  (flat =%.0f)\n", std::chrono::duration<double>(t3 - t2).count(), flat);
    printf("两者结果一致 = %s\n", (long long)grand == (long long)flat ? "YES" : "NO");
    return 0;
}

/**
 * 关键结论：
 * - TBB 的 auto 让【嵌套并行安全且高效】——内层 parallel_reduce 不会另起一批线程，
 *   而是复用同一工作窃取池；这是它比 OpenMP 嵌套并行（默认禁用、需 OMP_NESTED）
 *   更省心的地方。
 * - 但"能嵌套"不等于"该嵌套"：若外层已把并行度喂满，内层只是白增调度开销。
 *   本例平铺单层往往更快——嵌套用于【不规则/自适应】划分，不用于规整全表扫描。
 */
