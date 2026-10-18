/**
 * concurrent_queue：无锁任务队列（动态取任务的工作池）
 *
 * 原理：
 * 当"还有多少活"事先说不清（图遍历、爬虫、事件循环），最合适的是
 * 【工作池模式】：把待处理项塞进一个线程安全队列，多个 worker 不断 try_pop
 * 领任务，队列空了就退出。tbb::concurrent_queue 提供线程安全的
 * push / try_pop（非阻塞），比手搓 mutex+condition_variable 简洁高效。
 *
 * 编译指令：
 * g++ -O2 -std=c++17 concurrent_queue.cpp -o concurrent_queue $(pkg-config --cflags --libs tbb)
 *
 * 运行：
 * ./concurrent_queue
 */

#include <oneapi/tbb.h>
#include <oneapi/tbb/concurrent_queue.h>
#include <cstdio>
#include <atomic>

static const int NUM_TASKS = 200000;

int main() {
    tbb::concurrent_queue<int> tasks;
    for (int i = 1; i <= NUM_TASKS; ++i) tasks.push(i); // 生产阶段：全部入队

    std::atomic<long> done{0};
    std::atomic<long long> acc{0};

    // 消费阶段：起 concurrency 个 worker，每个循环 try_pop 直到队列空
    int workers = tbb::info::default_concurrency();
    tbb::parallel_for(
        tbb::blocked_range<int>(0, workers),
        [&](const tbb::blocked_range<int>& r) {
            for (int w = r.begin(); w != r.end(); ++w) {
                int item;
                // 每个 worker 独立排空队列；try_pop 在竞争下要么成功要么队列暂空
                while (tasks.try_pop(item)) {
                    acc += (long long)item * item; // 干活：求平方和
                    done++;
                }
            }
        });

    long long expect = 0;
    for (long i = 1; i <= NUM_TASKS; ++i) expect += i * i;
    printf("处理任务数 = %ld (期望 %d)，平方和 = %lld (期望 %lld)\n",
           done.load(), NUM_TASKS, acc.load(), expect);
    printf("正确 = %s\n",
           (done.load() == NUM_TASKS && acc.load() == expect) ? "YES" : "NO");
    return 0;
}

/**
 * 选型与注意：
 * - concurrent_queue 是【非阻塞】的：try_pop 空了就返回 false。做"生产者-消费者
 *   实时流水"（一边产一边消、消费者要等待）需要【阻塞语义】，应改用
 *   tbb::flow 的 queue_node / 或 concurrent_monitor，别用裸 try_pop 空转（费 CPU）。
 * - 本例把 acc/done 用 atomic 是合理的：更新频率是 O(任务数)，atomic 足够；
 *   若更新极高频，考虑每 worker 本地累加再合并（呼应 parallel_reduce）。
 * - 这个"共享队列 + 多 worker"就是一种显式工作窃取；TBB 的 task_group/parallel_for
 *   内部其实已做了类似的事——能用高层并行算法就别手搓队列，只在【任务动态、依赖不定】
 *   时才自己管队列。
 */
