/**
 * simple_partitioner：对分到 grainsize 即止，开销最小
 *
 * 原理：
 * simple_partitioner 只在【创建任务时】递归对分区间到 grainsize，叶子任务一旦开始
 * 执行就不再细分。相比 auto 少了运行时反复"检查能否再切"的开销，
 * 适合【迭代成本均匀、且不会被嵌套/窃取】的规整循环（如向量逐元素运算）。
 *
 * 与 auto 的分工：
 * - 负载均匀 + 单层并行 → simple 更快（省细分开销）
 * - 负载不均 / 需嵌套并行 → auto 更稳（可窃取、可再分）
 *
 * 编译指令：
 * g++ -O2 -std=c++17 simple_partitioner.cpp -o simple_partitioner $(pkg-config --cflags --libs tbb)
 *
 * 运行：
 * ./simple_partitioner
 */

#include <oneapi/tbb.h>
#include <cstdio>
#include <chrono>
#include <vector>

static const int N = 50000000;

int main() {
    std::vector<double> a(N, 1.0), b(N, 2.0), c(N, 0.0);

    auto bench = [&](const char* name, auto&& body) {
        auto t0 = std::chrono::high_resolution_clock::now();
        body();
        auto t1 = std::chrono::high_resolution_clock::now();
        printf("%-22s : %.4fs\n", name, std::chrono::duration<double>(t1 - t0).count());
    };

    // 规整的向量加：每迭代成本一致，simple 就够
    bench("simple_partitioner", [&] {
        tbb::parallel_for(tbb::blocked_range<int>(0, N, 10000),
            [&](const tbb::blocked_range<int>& r) {
                for (int i = r.begin(); i != r.end(); ++i) c[i] = a[i] + b[i];
            }, tbb::simple_partitioner());
    });
    bench("auto_partitioner(默认)", [&] {
        tbb::parallel_for(tbb::blocked_range<int>(0, N, 10000),
            [&](const tbb::blocked_range<int>& r) {
                for (int i = r.begin(); i != r.end(); ++i) c[i] = a[i] + b[i];
            }, tbb::auto_partitioner());
    });

    printf("c[N-1] = %.1f (期望 3)\n", c[N - 1]);
    return 0;
}

/**
 * 提醒：本例是内存带宽瓶颈（triad 型），simple/auto 差距不大且都受带宽限制；
 * 真正拉开差距的是"计算密集 + 有嵌套/不均"场景。别拿带宽瓶颈的差距去否定
 * 分区器选择的价值——换负载就要重测（第 30 课测量纪律）。
 */
