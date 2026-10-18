/**
 * 伪共享分析：多线程各写各的"独立槽位"，却被同一条缓存行串行化
 *
 * 原理（第 8 课实验⑥ / 第 7 课 MESI 的 TBB 版）：
 * 常见写法：给每个工作线程一个私有累加槽 result[tid]，逻辑上互不干扰。
 * 但若这些槽是【紧挨着的 double（8 字节）】，多个线程写的槽会落在【同一条 64B 缓存行】上。
 * MESI 下这条行的"独占权"在线程间疯狂乒乓（每写一次触发 RFO 失效对方副本），
 * 本该并行的写被迫串行 → "加了核反而更慢"的头号元凶。
 *
 * 本例故意用紧凑数组放大争用，作为 cache_aligned / reduction 两个正例的对照基线。
 *
 * 编译指令：
 * g++ -O2 -std=c++17 false_sharing_analysis.cpp -o false_sharing_analysis $(pkg-config --cflags --libs tbb)
 *
 * 运行：
 * ./false_sharing_analysis
 */

#include <oneapi/tbb.h>
#include <cstdio>
#include <chrono>
#include <vector>

static const int N = 40000000;

int main() {
    int slots = tbb::info::default_concurrency();
    // ❌ 紧凑布局：相邻槽位同处一条缓存行（8 个 double = 64B）
    std::vector<double> result(slots, 0.0);

    auto t0 = std::chrono::high_resolution_clock::now();
    // grainsize 设大 → 每个线程拿到一整段连续区间，期间固定写自己的 result[tid]
    tbb::parallel_for(
        tbb::blocked_range<int>(0, N, N / 200), // 约 200 块，够分又不细碎
        [&](const tbb::blocked_range<int>& r) {
            int tid = tbb::this_task_arena::current_thread_index();
            for (int i = r.begin(); i != r.end(); ++i)
                result[tid] += (i & 3) * 0.5; // 各写各的槽位，但槽位共享缓存行
        });
    auto t1 = std::chrono::high_resolution_clock::now();

    double total = 0;
    for (double x : result) total += x;
    printf("[紧凑数组·伪共享]  time=%.4fs  total=%.0f  (slots=%d)\n",
           std::chrono::duration<double>(t1 - t0).count(), total, slots);
    printf("→ 与 cache_aligned_counter / reduction_avoid_false_sharing 对比时间差。\n");
    return 0;
}

/**
 * 为什么 current_thread_index() 当槽位号是安全的？
 * - 同一次 body() 执行期间，任务不会被换线程，tid 稳定 → 没有数据竞争（各自独占槽位）。
 * - 问题只在【缓存行】这一物理层：逻辑独立的槽位被物理塞进同一行 → 伪共享。
 *
 * 三种消法（后两个文件各演示其一）：
 * 1) padding：把每个槽撑到一整条缓存行（alignas(64)）。
 * 2) tbb::cache_aligned_allocator：让分配按缓存行对齐 + 每槽独占一行。
 * 3) parallel_reduce：干脆不要共享数组，每任务寄存器私有累加、末尾合并（首选）。
 */
