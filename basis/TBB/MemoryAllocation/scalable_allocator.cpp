/**
 * scalable_allocator：多线程下的可扩展内存分配
 *
 * 原理：
 * 默认 malloc/new 内部有全局锁，多线程高频 alloc/free 会【在分配器上串行】——
 * 一个隐蔽但常见的并行瓶颈。TBB 提供 tbb::scalable_allocator（基于 Intel
 * Threading Building Blocks 的 libtbbmalloc，ptmalloc2 的可扩展替代品）：
 * - 每线程有本地空闲池，跨线程只在必要时同步 → 分配扩展性大增；
 * - 可直接当 STL 容器分配器：std::vector<T, tbb::scalable_allocator<T>>。
 *
 * 本例对比"默认分配器 vs scalable_allocator"在并行高频分配下的耗时。
 *
 * 编译指令（scalable_allocator 需要额外链接 libtbbmalloc）：
 * g++ -O2 -std=c++17 scalable_allocator.cpp -o scalable_allocator $(pkg-config --cflags --libs tbb) -ltbbmalloc
 *
 * 运行：
 * ./scalable_allocator
 */

#include <oneapi/tbb.h>
#include <oneapi/tbb/scalable_allocator.h>
#include <cstdio>
#include <chrono>
#include <vector>

static const int TASKS = 200000;   // 任务数
static const int ITEMS = 64;       // 每任务临时分配的元素数

int main() {
    // ---------- A：默认分配器 ----------
    auto ta0 = std::chrono::high_resolution_clock::now();
    volatile long guardA = 0;
    tbb::parallel_for(
        tbb::blocked_range<int>(0, TASKS, 2000),
        [&](const tbb::blocked_range<int>& r) {
            for (int t = r.begin(); t != r.end(); ++t) {
                std::vector<int> buf;              // 默认 new/malloc
                for (int k = 0; k < ITEMS; ++k) buf.push_back(t + k); // 反复扩容分配
                guardA += buf.front();
            }
        });
    auto ta1 = std::chrono::high_resolution_clock::now();

    // ---------- B：scalable_allocator ----------
    auto tb0 = std::chrono::high_resolution_clock::now();
    volatile long guardB = 0;
    tbb::parallel_for(
        tbb::blocked_range<int>(0, TASKS, 2000),
        [&](const tbb::blocked_range<int>& r) {
            for (int t = r.begin(); t != r.end(); ++t) {
                std::vector<int, tbb::scalable_allocator<int>> buf; // 可扩展分配
                for (int k = 0; k < ITEMS; ++k) buf.push_back(t + k);
                guardB += buf.front();
            }
        });
    auto tb1 = std::chrono::high_resolution_clock::now();

    double dA = std::chrono::duration<double>(ta1 - ta0).count();
    double dB = std::chrono::duration<double>(tb1 - tb0).count();
    printf("默认分配器      : %.4fs\n", dA);
    printf("scalable_allocator: %.4fs  (相对默认 %.2fx)\n", dB, dA / dB);
    printf("提示：若让全局 new 也走 scalable，可链接 -ltbbmalloc_proxy 运行。\n");
    return 0;
}

/**
 * 用法与取舍：
 * - 显式：容器第二模板参数换 tbb::scalable_allocator<T>（本例）。
 * - 全局：程序启动链接 libtbbmalloc_proxy，它会 hook malloc/free，让【未改代码】
 *   的 new/STL 也走 scalable（需 LD_PRELOAD 或链接顺序，视打包方式）。
 * - cache_aligned_allocator 是另一个：只负责"按缓存行对齐"，用于消伪共享（第 7 个文件），
 *   与 scalable_allocator（负责"分配扩展性"）解决的是不同问题，别混。
 * - 收益强依赖"分配有多频繁"：分配极少时看不出差别；换负载要重测（第 30 课）。
 */
