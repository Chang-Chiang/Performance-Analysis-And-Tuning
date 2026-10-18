/**
 * 用 tbb::cache_aligned_allocator 消除伪共享
 *
 * 原理：
 * 与 false_sharing_analysis 完全相同的算法，只改【数据布局】：
 * - tbb::cache_aligned_allocator 保证数组基址按缓存行（64B）对齐；
 * - 再把每个槽位的下标间隔放大到一整行：stride = cache_line_size / sizeof(double) = 8。
 * 于是 result[tid*stride] 各占独立缓存行，线程间不再乒乓 → 伪共享消失。
 *
 * 另一种等价写法：`struct alignas(64) Slot { double v; };`（或
 * alignas(std::hardware_destructive_interference_size)）让每个 Slot 本身就是
 * 一整行，用 std::vector<Slot> 天然隔离。
 *
 * 编译指令：
 * g++ -O2 -std=c++17 cache_aligned_counter.cpp -o cache_aligned_counter $(pkg-config --cflags --libs tbb)
 *
 * 运行：
 * ./cache_aligned_counter
 */

#include <oneapi/tbb.h>
#include <oneapi/tbb/cache_aligned_allocator.h>
#include <cstdio>
#include <chrono>
#include <vector>

static const int N = 40000000;
// 主流 x86 缓存行 64B。注：oneTBB 2021+ 不再导出公有 tbb::cache_line_size，
// 也可用 <new> 的 std::hardware_destructive_interference_size 代替。
constexpr std::size_t kCacheLine = 64;

int main() {
    int slots = tbb::info::default_concurrency();
    const std::size_t stride = kCacheLine / sizeof(double); // = 8：每槽独占一行

    // ✅ 缓存行对齐分配 + 每槽间隔一整行
    std::vector<double, tbb::cache_aligned_allocator<double>> result(stride * slots, 0.0);

    auto t0 = std::chrono::high_resolution_clock::now();
    tbb::parallel_for(
        tbb::blocked_range<int>(0, N, N / 200),
        [&](const tbb::blocked_range<int>& r) {
            int tid = tbb::this_task_arena::current_thread_index();
            double local = 0; // 先寄存器累加，出循环再落一次，进一步减少写次数
            for (int i = r.begin(); i != r.end(); ++i)
                local += (i & 3) * 0.5;
            result[tid * stride] += local; // 每槽独占缓存行，无乒乓
        });
    auto t1 = std::chrono::high_resolution_clock::now();

    double total = 0;
    for (int t = 0; t < slots; ++t) total += result[t * stride];
    printf("[cache_aligned 消伪共享]  time=%.4fs  total=%.0f  (stride=%zu)\n",
           std::chrono::duration<double>(t1 - t0).count(), total, stride);
    return 0;
}

/**
 * 注意本例还叠加了"寄存器私有累加 + 末尾一次写"的小优化：
 * 把每元素一次共享写，降到每任务一次共享写，争用次数骤减。
 * 即便如此，若不做行对齐，多次运行同一行仍会争用——所以两者互补：
 *   对齐（解决"同行"问题） + 私有累加（解决"高频写"问题）。
 * 而 parallel_reduce 把这两件事一次性帮你做对（见下一个文件），所以工程上首选它。
 */
