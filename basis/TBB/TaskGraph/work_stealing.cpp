/**
 * 工作窃取 vs 静态划分：负载不均时谁赢？
 *
 * 原理：
 * 造一个【越靠后的迭代越贵】的循环（cost(i) ∝ i），这是负载不均的极端情形：
 * - static_partitioner：把迭代均分成"线程数"个连续块，各线程各干一块，不互相插手。
 *   拿到"贵块"（靠后的大 i）的线程累死，拿到"便宜块"的线程早早闲坐 → 总时间=最慢块。
 * - auto_partitioner（默认）：递归对分 + 工作窃取。闲下来的线程会去偷忙线程
 *   【尚未执行】的子区间 → 自动把负载拉平。
 *
 * 这正是 OpenMP 里 schedule(static) vs schedule(dynamic) 的矛盾，
 * 只不过 TBB 把它做成"分区器（partitioner）"参数。
 *
 * 编译指令：
 * g++ -O2 -std=c++17 work_stealing.cpp -o work_stealing $(pkg-config --cflags --libs tbb)
 *
 * 运行：
 * ./work_stealing
 */

#include <oneapi/tbb.h>
#include <cstdio>
#include <chrono>
#include <vector>

static const int N = 10000000;
static volatile double sink = 0;

// 处理一个区间：对每个 i 做与 i 成正比的功 → 大 i 区间明显更贵
// 用函子而非普通函数：oneTBB 的 parallel_for 需要 Body 可作为数据成员拷贝，
// 直接传函数名会被推导成"函数类型"而非"函数指针"，导致 const Body 成员非法。
struct ProcessRange {
    void operator()(const tbb::blocked_range<int>& r) const {
        double local = 0;
        for (int i = r.begin(); i != r.end(); ++i) {
            int work = i / 500000; // i 越大，inner 循环越长 → 成本随 i 递增
            for (int t = 0; t <= work; ++t) local += t * 0.5;
        }
        sink += local;
    }
};

template <typename Partitioner>
double run(Partitioner p, const char* name) {
    auto t0 = std::chrono::high_resolution_clock::now();
    tbb::parallel_for(tbb::blocked_range<int>(0, N, 100000), ProcessRange(), p);
    auto t1 = std::chrono::high_resolution_clock::now();
    double sec = std::chrono::duration<double>(t1 - t0).count();
    printf("%-18s : %.4fs\n", name, sec);
    return sec;
}

int main() {
    printf("并发度 = %d，负载随 i 递增（越靠后越贵）\n", tbb::info::default_concurrency());
    double t_static = run(tbb::static_partitioner(), "static(不窃取)");
    double t_auto   = run(tbb::auto_partitioner(),   "auto(工作窃取)");
    printf("→ auto 相对 static 提速 %.2fx（负载越不均，差距越大）\n", t_static / t_auto);
    return 0;
}

/**
 * 四种分区器速查（LoadBalancing/ 目录各有专例）：
 * - auto   ：默认。递归对分 + 运行时窃取，最省心，绝大多数场景首选。
 * - simple ：对分到 grainsize 为止，不再运行时细分；开销比 auto 小，适合无复用/无嵌套。
 * - static ：恰好切成"线程数"个连续块，无窃取。块耗时均衡时开销最低，不均时最惨。
 * - affinity：跨多次调用记住"哪块曾在哪个线程跑过"，复用缓存（见 affinity_partitioner）。
 *
 * 经验法则：默认 auto；只有实测发现 auto 的细分开销拖累你，才降级到 simple/static。
 */
