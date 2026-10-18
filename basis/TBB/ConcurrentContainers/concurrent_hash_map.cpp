/**
 * concurrent_hash_map：并发计数（key 会被多线程命中）
 *
 * 原理：
 * 词频统计/直方图这类任务里，不同线程会往【同一个 key】上累加——这是【真共享】
 * （同一逻辑数据被并发写），靠 padding 消不掉，必须用并发安全容器或归约。
 * tbb::concurrent_hash_map 用【细粒度、按桶的读写锁】：
 * - accessor（写访问器）持锁期间可安全改 value；
 * - const_accessor（读访问器）多读者可并行。
 * 比"全局一把 std::mutex"扩展性好得多。
 *
 * 编译指令：
 * g++ -O2 -std=c++17 concurrent_hash_map.cpp -o concurrent_hash_map $(pkg-config --cflags --libs tbb)
 *
 * 运行：
 * ./concurrent_hash_map
 */

#include <oneapi/tbb.h>
#include <oneapi/tbb/concurrent_hash_map.h>
#include <cstdio>

static const int N = 8000000;
static const int NUM_BUCKETS = 100;

using Counter = tbb::concurrent_hash_map<int, int>;

int main() {
    Counter freq;

    tbb::parallel_for(
        tbb::blocked_range<int>(0, N, 200000),
        [&](const tbb::blocked_range<int>& r) {
            // 任务内先在本地 unordered 累加，再批量并入共享表 → 大幅减少持锁次数
            int local[NUM_BUCKETS] = {0};
            for (int i = r.begin(); i != r.end(); ++i) local[i % NUM_BUCKETS]++;
            for (int b = 0; b < NUM_BUCKETS; ++b) {
                if (!local[b]) continue;
                Counter::accessor a;
                freq.insert(a, b); // 不存在则插入；存在则定位
                a->second += local[b]; // 持锁原子累加
            }
        });

    // 校验：总数应为 N，每个桶应为 N/NUM_BUCKETS
    long total = 0;
    for (auto& kv : freq) total += kv.second;
    int v0 = 0;
    { Counter::const_accessor a; if (freq.find(a, 0)) v0 = a->second; }
    printf("并发计数总命中 = %ld (期望 %d)，桶0 = %d (期望 %d)\n",
           total, N, v0, N / NUM_BUCKETS);
    printf("正确 = %s\n", (total == N && v0 == N / NUM_BUCKETS) ? "YES" : "NO");
    return 0;
}

/**
 * 关键取舍：
 * - "本地累加 + 批量并入共享表"是本例的核心优化：把 per-element 持锁降到
 *   per-task，锁竞争骤减。若确实高频命中同一 key，还应考虑：
 *   1) parallel_reduce 归约出每线程局部直方图，最后串行合并（最快）；
 *   2) 分片（sharding）：按 key 段切成多张小表，降低同桶概率。
 * - concurrent_hash_map 适合【并发读写、key 动态、需在线查询】；
 *   纯离线计数用 reduction 往往更简单更快。选型看访问模式，别默认"并发就上并发容器"。
 */
