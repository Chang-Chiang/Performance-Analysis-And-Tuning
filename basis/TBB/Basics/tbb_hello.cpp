/**
 * 第一个 TBB 程序
 *
 * 原理：
 * 展示 Intel oneTBB（Threading Building Blocks）的基本使用方法：
 * - tbb::parallel_for：把循环交给任务调度器（不是把某段代码包进线程）
 * - tbb::blocked_range：把迭代空间切成可递归对分的区间
 * - tbb::info::default_concurrency()：查询默认可用并发度（≈ 逻辑核数）
 * - 与 OpenMP 的本质区别：OpenMP 面向"线程"，TBB 面向"任务"，
 *   任务由工作窃取（work-stealing）调度器自动映射到线程。
 *
 * 编译指令（需要 oneTBB，pkg-config 指向 Intel oneTBB）：
 * g++ -O2 -std=c++17 tbb_hello.cpp -o tbb_hello $(pkg-config --cflags --libs tbb)
 * 若系统只有 apt 版：g++ -O2 -std=c++17 tbb_hello.cpp -o tbb_hello -ltbb
 *
 * 运行：
 * ./tbb_hello
 */

#include <oneapi/tbb.h>
#include <cstdio>
#include <vector>

#define N 1e6

int main() {
    // 查询 TBB 默认并发度：本机 Intel Ultra 7 270K Plus（8 P核 + 16 E核）通常报告 24
    int concurrency = tbb::info::default_concurrency();
    printf("TBB default_concurrency = %d\n\n", concurrency);

    std::vector<int> A(N), B(N, 1), C(N);
    for (int i = 0; i < N; ++i) A[i] = i;

    // parallel_for：把 [0, N) 切成 blocked_range 交给任务调度器并行执行
    // 与 OpenMP 的 #pragma omp parallel for 相比：这里是真正的库调用，
    // 粒度（grainsize=4）由我们给定下限，调度器再按需递归对分。
    tbb::parallel_for(
        tbb::blocked_range<int>(0, N, /*grainsize=*/4),
        [&](const tbb::blocked_range<int>& r) {
            for (int i = r.begin(); i != r.end(); ++i) {
                C[i] = A[i] + B[i];
                // 打印处理该元素的工作线程 ID（同一元素在不同运行中可能落到不同线程）
                printf("worker %2d compute A[%d]+B[%d] = %d\n",
                       tbb::this_task_arena::current_thread_index(), i, i, C[i]);
            }
        });

    return 0;
}

/**
 * TBB 与 OpenMP 的关键差异（本例可体会）：
 *
 * 1. 抽象层级：
 *    - OpenMP：并行区 = 一组线程，程序员关心"几个线程"。
 *    - TBB：并行 = 一组任务，程序员只描述"怎么切分 + 每块干什么"，
 *      "用几个线程"交给运行时的工作窃取调度器决定。
 *
 * 2. 变量作用域：
 *    - OpenMP 要手写 shared/private/reduction 子句（第 8 课头号坑）。
 *    - TBB 用 C++ 闭包 `[&]` 捕获：外部变量天然共享，循环体内局部变量天然私有，
 *      作用域规则就是普通 C++，无需额外标注。
 *
 * 3. current_thread_index()：
 *    - 类比 omp_get_thread_num()，但它是"工作线程槽位号"，
 *      任务可能在两次运行中落到不同槽位——这正体现"任务 ≠ 线程"。
 */
