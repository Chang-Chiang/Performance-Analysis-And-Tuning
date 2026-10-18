/**
 * parallel_invoke（并行独立任务）+ parallel_sort（并行排序）
 *
 * 原理：
 * - parallel_invoke：把若干【相互独立】的可调用对象同时跑，全部完成才返回。
 *   适合"任务图是几棵独立子树"的场景（如并行加载多个模块），无需自己建线程。
 * - parallel_sort：内置的并行不稳定排序（分治 + 插入排序处理小块），
 *   对大数据能近线性加速，是小规模 std::sort 的直接替代品。
 *
 * 编译指令：
 * g++ -O2 -std=c++17 parallel_invoke_sort.cpp -o parallel_invoke_sort $(pkg-config --cflags --libs tbb)
 *
 * 运行：
 * ./parallel_invoke_sort
 */

#include <oneapi/tbb.h>
#include <cstdio>
#include <vector>
#include <chrono>
#include <random>
#include <algorithm>

// 三个互相独立的"任务"：parallel_invoke 会并行跑它们
static long taskA() { long s = 0; for (int i = 0; i < 20000000; ++i) s += i % 7; return s; }
static long taskB() { long s = 0; for (int i = 0; i < 20000000; ++i) s += i % 5; return s; }
static long taskC() { long s = 0; for (int i = 0; i < 20000000; ++i) s += i % 3; return s; }

int main() {
    // ---------- parallel_invoke ----------
    long ra = 0, rb = 0, rc = 0;
    auto t0 = std::chrono::high_resolution_clock::now();
    tbb::parallel_invoke(
        [&] { ra = taskA(); },
        [&] { rb = taskB(); },
        [&] { rc = taskC(); });
    auto t1 = std::chrono::high_resolution_clock::now();
    double par = std::chrono::duration<double>(t1 - t0).count();

    // 串行对照
    t0 = std::chrono::high_resolution_clock::now();
    ra = taskA(); rb = taskB(); rc = taskC();
    t1 = std::chrono::high_resolution_clock::now();
    double ser = std::chrono::duration<double>(t1 - t0).count();
    printf("parallel_invoke: 串行 %.3fs vs 并行 %.3fs  → 加速 %.2fx\n", ser, par, ser / par);

    // ---------- parallel_sort ----------
    const int N = 5000000;
    std::mt19937 rng(42);
    std::vector<int> base(N);
    for (int& x : base) x = rng();

    std::vector<int> v1 = base;
    t0 = std::chrono::high_resolution_clock::now();
    std::sort(v1.begin(), v1.end());
    t1 = std::chrono::high_resolution_clock::now();
    double t_std = std::chrono::duration<double>(t1 - t0).count();

    std::vector<int> v2 = base;
    t0 = std::chrono::high_resolution_clock::now();
    tbb::parallel_sort(v2.begin(), v2.end());
    t1 = std::chrono::high_resolution_clock::now();
    double t_tbb = std::chrono::duration<double>(t1 - t0).count();

    bool sorted_ok = std::is_sorted(v2.begin(), v2.end());
    printf("parallel_sort N=%d: std::sort %.3fs vs TBB %.3fs → 加速 %.2fx (已排序=%s)\n",
           N, t_std, t_tbb, t_std / t_tbb, sorted_ok ? "YES" : "NO");

    return 0;
}

/**
 * 选型提示：
 * - parallel_invoke 的每个可调用对象是【整块任务】，内部不再自动切分；
 *   若某个任务本身还能数据并行，应在其内部再调用 parallel_for（任务嵌套/递归）。
 * - parallel_sort 不稳定（相等元素相对顺序可能变）。需要稳定性请自己实现
 *   "分段 std::stable_sort + 并行归并"或用索引排序。
 * - 小数组（几千以内）parallel_sort 可能不比 std::sort 快——任务创建有常数开销，
 *   呼应第 8 课"小任务并行反而亏"。
 */
