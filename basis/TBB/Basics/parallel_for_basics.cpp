/**
 * parallel_for 的两种写法
 *
 * 原理：
 * TBB 的并行算法本质是"对可递归对分的区间做分治"。同一件事有两种表达：
 * - 写法 A（现代/推荐）：lambda 闭包，一行搞定；
 * - 写法 B（经典 body）：实现 operator() + split 构造函数的可拷贝对象，
 *   是理解 TBB "任务递归对分"机制的钥匙，也是并行归约/扫描的通用范式。
 *
 * 编译指令：
 * g++ -O2 -std=c++17 parallel_for_basics.cpp -o parallel_for_basics $(pkg-config --cflags --libs tbb)
 *
 * 运行：
 * ./parallel_for_basics
 */

#include <oneapi/tbb.h>
#include <cstdio>
#include <vector>

#define N 1000000

// ---------- 写法 B：经典 body（函数对象） ----------
// body 必须提供：
//   1) split 构造函数：把一个 body 拆成两半（this 处理 [begin,mid)，b 处理 [mid,end)）
//   2) operator()(range)：处理给定区间
// TBB 在需要更多并行度时，会自动反复调用 split 构造对分区间。
struct ScalarFill {
    
    std::vector<double>& out;

    // 完整区间构造
    ScalarFill(std::vector<double>& o) : out(o) {}

    // 对分构造（split）：this 与 b 共享同一底层数据，但各管一段区间
    ScalarFill(ScalarFill& b, tbb::split) : out(b.out) {}

    void operator()(const tbb::blocked_range<size_t>& r) const {
        for (size_t i = r.begin(); i != r.end(); ++i)
            out[i] = i * 1.5;
    }
};

int main() {
    std::vector<double> a(N), b(N);

    // ---------- 写法 A：lambda（最常用） ----------
    // 把 [0,N) 交给 parallel_for，闭包按引用捕获 a。grainsize 给一个合理下限，
    // 调度器会在动态范围内继续对分以匹配实际线程数。
    tbb::parallel_for(
        tbb::blocked_range<int>(0, N, 1000),
        [&](const tbb::blocked_range<int>& r) {
            for (int i = r.begin(); i != r.end(); ++i)
                a[i] = i * 1.5;
        });

    // ---------- 写法 B：经典 body ----------
    ScalarFill filler(b);
    tbb::parallel_for(tbb::blocked_range<size_t>(0, N, 1000), filler);

    // 校验两种写法结果一致
    bool same = true;
    for (int i = 0; i < N; ++i)
        if (a[i] != b[i]) { same = false; break; }
    printf("lambda form == classic body form : %s\n", same ? "YES" : "NO");
    printf("a[N-1] = %.1f (expect %.1f)\n", a[N - 1], (N - 1) * 1.5);

    return 0;
}

/**
 * 什么时候用哪种写法？
 *
 * - lambda：绝大多数场景。简洁、不易出错，作用域靠 C++ 捕获规则天然正确。
 * - 经典 body：需要"每线程私有累加 + 末尾合并"的归约/扫描（见 parallel_reduce /
 *   parallel_scan），此时 split 构造 + assign/reverse_join 是必须手写的接口；
 *   读老代码或面试时也常遇到。
 *
 * 关键机制——递归对分（recursive fission）：
 *   区间 [0,N) → [0,N/2)+[N/2,N) → ... 直到达到 grainsize 或用完线程。
 *   空出来的线程会从别的线程"偷"尚未执行的任务区间 → 天然负载均衡。
 */
