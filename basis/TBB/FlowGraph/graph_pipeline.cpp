/**
 * flow_graph：用"有向图"表达任务依赖（TBB 的组合并行利器）
 *
 * 原理：
 * 当并行结构不是规整循环，而是"节点 + 依赖边"的 DAG（预处理 → 计算 → 后处理，
 * 或多路扇入扇出），用 tbb::flow 建一张【流图】最自然：节点是任务，边是数据流，
 * graph.wait_for_all() 触发并等待整图跑完。节点内部自动并行、自动流水重叠——
 * 正是第 21 课母题①"把不同阶段在不同 item 上叠起来掩盖延迟"。
 *
 * 关键概念：
 * - input_node：源源不断产生消息（数据源），用 flow_control::set_stop() 停止。
 * - function_node<In,Out>(g, concurrency, body)：concurrency 限制同时活跃实例数
 *   （unlimited / 具体数）→ 控制并行度与资源占用。
 * - make_edge / join_node：连线 / 多路扇入同步。
 *
 * 编译指令：
 * g++ -O2 -std=c++17 graph_pipeline.cpp -o graph_pipeline $(pkg-config --cflags --libs tbb)
 *
 * 运行：
 * ./graph_pipeline
 */

#include <oneapi/tbb.h>
#include <oneapi/tbb/flow_graph.h>
#include <cstdio>
#include <chrono>
#include <cmath>
#include <atomic>

static const int NUM = 3000;

int main() {
    std::atomic<long> produced{0};
    double consumed = 0; // sink 并发度=1，串行写，无需原子

    auto t0 = std::chrono::high_resolution_clock::now();

    tbb::flow::graph g;

    // 源：产生 0..NUM-1
    tbb::flow::input_node<int> src(g, [&](tbb::flow_control& fc) -> int {
        long n = produced++;
        if (n < NUM) return (int)n;
        fc.stop(); // oneTBB：flow_control::stop()（老 TBB 叫 set_stop）
        return -1;
    });

    // 计算节点：并发度不限（可同时处理多个 item）；做一段较重变换
    tbb::flow::function_node<int, double> work(g, tbb::flow::unlimited,
        [](int x) { double s = 0; for (int i = 1; i <= 20000; ++i) s += std::sqrt(x + i); return s; });

    // 汇聚节点：并发度=1，串行累加（避免对 consumed 的竞争）
    tbb::flow::function_node<double> sink(g, /*concurrency=*/1,
        [&](double v) { consumed += v; });

    tbb::flow::make_edge(src, work);
    tbb::flow::make_edge(work, sink);

    src.activate();      // 启动源
    g.wait_for_all();    // 等待整张图排空

    auto t1 = std::chrono::high_resolution_clock::now();
    printf("flow_graph: %d 个 item 经 3 节点图处理，用时 %.4fs，累加=%.3g\n",
           NUM, std::chrono::duration<double>(t1 - t0).count(), consumed);

    // 串行基线对照
    auto t2 = std::chrono::high_resolution_clock::now();
    double ss = 0;
    for (int x = 0; x < NUM; ++x) { double s = 0; for (int i = 1; i <= 20000; ++i) s += std::sqrt(x + i); ss += s; }
    auto t3 = std::chrono::high_resolution_clock::now();
    double ser = std::chrono::duration<double>(t3 - t2).count();
    double par = std::chrono::duration<double>(t1 - t0).count();
    printf("串行 %.4fs → 流图加速 %.2fx\n", ser, ser / par);
    return 0;
}

/**
 * flow_graph vs parallel_pipeline vs parallel_for：
 * - parallel_for：所有 item 走同一函数、无阶段之分 → 用它，最省。
 * - parallel_pipeline：固定线性多阶段（A→B→C）流水 → 用它。
 * - flow_graph：任意 DAG（分支、扇入扇出、循环反馈、动态构造）→ 表达力最强，
 *   但要自己管节点与边，心智负担更大。够用就好，别一上来就建图。
 * - 记住：flow_graph 的节点并发度要显式设，sink 设 1 保证累加无竞争；
 *   若设 unlimited 且内部写共享量，就会数据竞争（结果会乱）。
 */
