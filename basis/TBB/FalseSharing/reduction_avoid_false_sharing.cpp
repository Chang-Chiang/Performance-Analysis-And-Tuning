/**
 * 用 parallel_reduce 消除伪共享（工程首选）
 *
 * 原理：
 * 前两个文件在"如何摆放私有槽位"上做文章；其实更根本的修正是：
 * 归约类需求【压根不该有一个被多线程反复写的共享数组】。
 * parallel_reduce 让每个任务在【自己的寄存器/局部变量】里累加，只在合并阶段
 * 做一次加法归并——既没有共享写，也就没有伪共享，还比手撸 padding 更简洁省内存。
 *
 * 对照第 8 课：OpenMP 的 reduction(+:x) 是同一思想；这里三个文件给出
 * "紧凑数组(慢) → cache_aligned padding → reduction(最优)" 的完整阶梯。
 *
 * 编译指令：
 * g++ -O2 -std=c++17 reduction_avoid_false_sharing.cpp -o reduction_avoid_false_sharing $(pkg-config --cflags --libs tbb)
 *
 * 运行：
 * ./reduction_avoid_false_sharing
 */

#include <oneapi/tbb.h>
#include <cstdio>
#include <chrono>

static const int N = 40000000;

int main() {
    auto t0 = std::chrono::high_resolution_clock::now();
    // 每个子区间在私有 double 上累加，combine 只在树上合并 O(任务数) 次
    double total = tbb::parallel_reduce(
        tbb::blocked_range<int>(0, N, N / 200), /*init=*/0.0,
        [&](const tbb::blocked_range<int>& r, double local) {
            for (int i = r.begin(); i != r.end(); ++i)
                local += (i & 3) * 0.5;
            return local;
        },
        [](double a, double b) { return a + b; });
    auto t1 = std::chrono::high_resolution_clock::now();

    printf("[parallel_reduce 消伪共享]  time=%.4fs  total=%.0f\n",
           std::chrono::duration<double>(t1 - t0).count(), total);
    printf("total 应与 false_sharing_analysis / cache_aligned_counter 一致，但更快、更省内存。\n");
    return 0;
}

/**
 * 为什么这是"首选"？
 * - 无共享写：根本不构造 result[tid] 数组，从源头消除伪共享与真竞争。
 * - 无内存浪费：padding 方案要为每个槽浪费 56 字节的填充；reduction 不需要。
 * - 可组合：combine 可换成任意满足结合律的算子（max、向量相加、直方图合并……）。
 *
 * 何时仍要用 padding/cache_aligned？当"每线程结果"本身就是要【保留下来分别查看】
 * （如每线程独立统计分布、各线程自己的缓冲区），而非归约成一个标量时。
 */
