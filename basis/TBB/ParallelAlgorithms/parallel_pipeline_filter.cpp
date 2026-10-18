/**
 * parallel_pipeline（流式管道：三阶段过滤）
 *
 * 原理：
 * 面向"数据流"式并行：把处理拆成【顺序固定的多个阶段（filter）】，
 * 每个阶段内部可同时处理多个 item（跨 item 流水并行），从而把
 * "取数/IO/计算/落盘"在不同 item 上重叠——正是第 21 课母题①"掩盖延迟"。
 *
 * filter 三种模式：
 * - serial_in_order / serial_out_of_order：该阶段一次只处理一个 item（保序/不保序）
 * - parallel：该阶段可同时处理多个 item（吞吐瓶颈所在，放重活）
 *
 * ⚠️ oneTBB 里 make_filter 是 make_filter<In,Out>(mode, body)，
 *    老 TBB 4.x 的 `make_filter<In,Out>().filter(...)` 链式写法已移除。
 *
 * 编译指令：
 * g++ -O2 -std=c++17 parallel_pipeline_filter.cpp -o parallel_pipeline_filter $(pkg-config --cflags --libs tbb)
 *
 * 运行：
 * ./parallel_pipeline_filter
 */

#include <oneapi/tbb.h>
#include <cstdio>
#include <chrono>
#include <cmath>

static const int NUM_ITEMS = 1000;

// 模拟一段较重的逐 item 计算（放并行阶段才能吃到流水收益）
static double heavy(int x) { double s = 0; for (int i = 1; i <= 20000; ++i) s += std::sqrt(x + i); return s; }

int main() {
    // ---------- 串行基线：一个 item 从头到尾顺序做完再做下一个 ----------
    auto t0 = std::chrono::high_resolution_clock::now();
    double serial_sum = 0;
    for (int i = 0; i < NUM_ITEMS; ++i) serial_sum += heavy(i);
    auto t1 = std::chrono::high_resolution_clock::now();
    double serial = std::chrono::duration<double>(t1 - t0).count();

    // ---------- 三阶段流水线 ----------
    int produced = 0;
    double pipe_sum = 0;
    t0 = std::chrono::high_resolution_clock::now();
    tbb::parallel_pipeline(
        /*max_number_of_inflight_items=*/tbb::info::default_concurrency() * 2, // 同时在途 item 数
        // 阶段1：生产 item（serial_in_order，一次一个，控制停止）
        tbb::make_filter<void, int>(
            tbb::filter_mode::serial_in_order,
            [&](tbb::flow_control& fc) -> int {
                if (produced < NUM_ITEMS) return produced++;
                fc.stop(); // oneTBB：flow_control::stop()（老 TBB 叫 set_stop）
                return -1;
            })
        // 阶段2：重计算（parallel：可同时处理多个 item）
      & tbb::make_filter<int, double>(
            tbb::filter_mode::parallel,
            [](int x) { return heavy(x); })
        // 阶段3：消费（serial_in_order 保序累加，或 serial_out_of_order 提高吞吐）
      & tbb::make_filter<double, void>(
            tbb::filter_mode::serial_out_of_order,
            [&](double v) { pipe_sum += v; }));
    t1 = std::chrono::high_resolution_clock::now();
    double pipe = std::chrono::duration<double>(t1 - t0).count();

    printf("串行           : %.3fs\n", serial);
    printf("parallel_pipeline: %.3fs  → 加速 %.2fx\n", pipe, serial / pipe);
    printf("结果一致性: |serial-pipe| = %.6g\n", std::fabs(serial_sum - pipe_sum));
    return 0;
}

/**
 * 何时用 pipeline、何时用 parallel_for？
 * - 各 item 走【相同的多阶段流程】、且阶段间要保序/复用昂贵资源（如解码→处理→编码）
 *   → parallel_pipeline，天然把不同 item 的不同阶段重叠。
 * - 只是"同一函数打遍所有数据"→ parallel_for 更简单。
 *
 * max_number_of_inflight_items 设太小→ 无法充分流水；太大→ 内存占用与调度开销上升。
 * 常取"并发度 × 若干"作起点，再按实测调。
 */
