/**
 * task_group：递归分治 + 动态任务 + 工作窃取
 *
 * 原理：
 * TBB 最强的地方是【运行时动态生成任务】——递归里随时 g.run() 派生子任务，
 * 调度器用工作窃取把空闲线程填上。这与 OpenMP 的 fork-join（固定并行区）
 * 形成鲜明对比：任务树有多少节点就能榨多少并行，粒度自适应。
 *
 * 经典例子：递归 Fibonacci（严重重复计算，天然二叉任务树）。
 *
 * ⚠️ 血泪坑：递归任务【必须有串行 cutoff】。若每个 n 都无脑 g.run 两个子任务，
 *    fib(30) 会派生上千万任务 → 递归栈 + 任务对象爆内存/栈溢出直接段错误
 *    （本示例去掉 cutoff 实测会 SIGSEGV）。到某个规模以下就串行算。
 *
 * 编译指令：
 * g++ -O2 -std=c++17 task_group_fibonacci.cpp -o task_group_fibonacci $(pkg-config --cflags --libs tbb)
 *
 * 运行：
 * ./task_group_fibonacci
 */

#include <oneapi/tbb.h>
#include <oneapi/tbb/task_group.h>
#include <cstdio>
#include <chrono>

static const int CUTOFF = 25; // n < CUTOFF 走纯串行，防止任务爆炸

// 纯串行 fib（基线）
static long fib_serial(long n) { return n < 2 ? n : fib_serial(n - 1) + fib_serial(n - 2); }

// 任务并行 fib：到 cutoff 退回串行，否则派生一个子任务 + 本地做一个
static long fib_tbb(long n) {
    if (n < 2) return n;
    if (n < CUTOFF) return fib_serial(n); // 关键：cutoff，否则段错误
    long f1, f2;
    tbb::task_group g;
    g.run([&] { f1 = fib_tbb(n - 1); }); // 异步派生左子树
    f2 = fib_tbb(n - 2);                 // 当前线程做右子树
    g.wait();                            // 等待左子树（隐式 join）
    return f1 + f2;
}

int main() {
    const long N = 35;

    auto t0 = std::chrono::high_resolution_clock::now();
    long fs = fib_serial(N);
    auto t1 = std::chrono::high_resolution_clock::now();
    double ser = std::chrono::duration<double>(t1 - t0).count();

    t0 = std::chrono::high_resolution_clock::now();
    long fp = fib_tbb(N);
    t1 = std::chrono::high_resolution_clock::now();
    double par = std::chrono::duration<double>(t1 - t0).count();

    printf("fib(%ld): 串行 %.3fs  vs  task_group 并行 %.3fs → 加速 %.2fx (结果 %ld, 一致=%s)\n",
           N, ser, par, ser / par, fp, (fs == fp) ? "YES" : "NO");
    printf("提示：把 CUTOFF 改成 0 再跑，体会任务爆炸导致的栈溢出/段错误。\n");
    return 0;
}

/**
 * g.wait() vs 不 wait：
 * - g.run() 只【派生】任务，不保证完成；必须 g.wait() 才能读到子任务写的 f1。
 * - task_group 析构前隐式 wait，但显式 wait 更清晰、也能尽早暴露异常。
 *
 * 对照 OpenMP：task_group ≈ omp task + taskwait，但 TBB 的任务粒度和线程映射
 * 完全由工作窃取调度器决定，无需 OMP_NUM_THREADS 也能良好伸缩。
 */
