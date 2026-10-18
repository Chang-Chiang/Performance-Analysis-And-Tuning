/**
 * affinity_partitioner：跨多次迭代复用"块↔线程"绑定
 *
 * 原理：
 * 许多算法是"同一份数据、反复迭代更新"（时间步、Jacobi/Gauss-Seidel 松弛、
 * 迭代求解器……）。每次 parallel_for 若随机重排块到线程，缓存/NUMA 局部性会被打散，
 * 每步都要把数据重新拉回本地。affinity_partitioner 会【记住上一次哪个块落在哪个线程】，
 * 在后续迭代里尽量把同一块放回同一线程 → 复用其在本地缓存里的副本。
 *
 * 用法要点：affinity_partitioner 对象必须【跨多次 parallel_for 长期存活并复用】，
 *           第一次调用只是"建立亲和"，从第二次起才吃到收益。
 *
 * 编译指令：
 * g++ -O2 -std=c++17 affinity_partitioner.cpp -o affinity_partitioner $(pkg-config --cflags --libs tbb)
 *
 * 运行：
 * ./affinity_partitioner
 */

#include <oneapi/tbb.h>
#include <cstdio>
#include <chrono>
#include <vector>

static const int N = 20000000;
static const int STEPS = 12;

// 一维三点平滑（stencil）：在两个缓冲间反复扫，逐时间步更新
// 数据布局每步都不变 → 正是 affinity 的用武之地
static void sweep(std::vector<float>& cur, std::vector<float>& nxt) {
    tbb::affinity_partitioner ap; // 关键：对象在多次调用间复用
    double total = 0;
    for (int s = 0; s < STEPS; ++s) {
        auto t0 = std::chrono::high_resolution_clock::now();
        tbb::parallel_for(
            tbb::blocked_range<int>(1, N - 1, 100000),
            [&](const tbb::blocked_range<int>& r) {
                for (int i = r.begin(); i != r.end(); ++i)
                    nxt[i] = (cur[i - 1] + cur[i] + cur[i + 1]) * (1.0f / 3.0f);
            },
            ap); // 复用同一个 ap → 后续步倾向把同段放回同线程
        auto t1 = std::chrono::high_resolution_clock::now();
        double sec = std::chrono::duration<double>(t1 - t0).count();
        total += sec;
        printf("step %2d : %.4fs%s\n", s, sec, s == 0 ? "   \u2190 首步：建立亲和，无复用" : "");
        cur.swap(nxt); // 双缓冲交换，无额外拷贝
    }
    printf("平均 %.4fs/步\n", total / STEPS);
}

int main() {
    std::vector<float> a(N, 0.f), b(N, 0.f);
    a[N / 2] = 1000.f; // 初始一个脉冲，后续被逐步平滑
    printf("并发度 = %d\n", tbb::info::default_concurrency());
    sweep(a, b);
    return 0;
}

/**
 * 何时值得用 affinity？
 * - 有【明确的多轮迭代 + 数据基本不变位置】→ 收益明显（缓存/NUMA 命中）。
 * - 单次 parallel_for 或数据每步被打乱 → 用不上（它靠"历史"才有用）。
 *
 * 对照 OpenMP：类似 OMP_PLACES/OMP_PROC_BIND 把线程钉在固定核上，但 TBB 把
 * "块"而非"线程"绑到核，粒度更细、更能顺应工作窃取框架。
 * 提示：若把上面的 `ap` 换成 auto_partitioner，首步之外的"绑定记忆"就没了。
 */
