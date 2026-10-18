/**
 * concurrent_vector：多线程并发"追加"而不加锁
 *
 * 原理：
 * 结果数量事先不知道（过滤/搜索/事件收集），多线程都想往同一个列表 push。
 * - std::vector：并发 push_back 会数据竞争 → 要么加锁（慢），要么每线程私有再合并。
 * - tbb::concurrent_vector：内部按块管理，支持多个线程并发【在尾部各自追加一段】而
 *   无全局锁；用 grow_by / unchecked_push_back 拿到一段专属下标区间，几乎零争用。
 *
 * 注意：并发 grow 的【顺序不确定】（谁先申请到哪块就落哪），但【元素集合】正确；
 *      若下游依赖顺序，先并行填充再 parallel_sort。
 *
 * 编译指令：
 * g++ -O2 -std=c++17 concurrent_vector.cpp -o concurrent_vector $(pkg-config --cflags --libs tbb)
 *
 * 运行：
 * ./concurrent_vector
 */

#include <oneapi/tbb.h>
#include <oneapi/tbb/concurrent_vector.h>
#include <cstdio>
#include <chrono>
#include <vector>

static const int N = 5000000;

int main() {
    tbb::concurrent_vector<int> hits; // 并发安全的动态数组

    auto t0 = std::chrono::high_resolution_clock::now();
    tbb::parallel_for(
        tbb::blocked_range<int>(0, N, 100000),
        [&](const tbb::blocked_range<int>& r) {
            // 任务内先攒到本地小缓冲，再一次 grow_by → 减少全局尾部争用（关键技巧）
            std::vector<int> local;
            for (int i = r.begin(); i != r.end(); ++i)
                if (i % 7 == 0 && i % 3 != 0) local.push_back(i); // 收集满足条件者
            if (!local.empty()) hits.grow_by(local.begin(), local.end());
        });
    auto t1 = std::chrono::high_resolution_clock::now();

    // 校验：串行数一遍应有多少个
    long expect = 0;
    for (int i = 0; i < N; ++i) if (i % 7 == 0 && i % 3 != 0) ++expect;
    printf("concurrent_vector 命中数 = %zu, 期望 = %ld, 正确 = %s\n",
           hits.size(), expect, ((long)hits.size() == expect) ? "YES" : "NO");
    printf("并行收集耗时 = %.4fs（无锁、无预分配）\n",
           std::chrono::duration<double>(t1 - t0).count());

    // 下游要有序？再并行排序（顺序与插入无关，集合已正确）
    tbb::parallel_sort(hits.begin(), hits.end());
    printf("排序后前 5 个 = %d %d %d %d %d\n", hits[0], hits[1], hits[2], hits[3], hits[4]);
    return 0;
}

/**
 * 关键工程点：
 * - 别在热循环里逐个 unchecked_push_back（每次都要原子申请尾部，争用高）；
 *   本例用"本地攒 + 批量 grow_by"把全局操作次数降到 O(任务数)。
 * - concurrent_vector 迭代器在 grow 后可能失效（类似 std::vector realloc）；
 *   要长期持有请用索引，别缓存指针。
 */
