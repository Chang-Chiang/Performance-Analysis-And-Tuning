/**
 * parallel_reduce 并行归约（求和）
 *
 * 原理：
 * 归约（sum/max/min/...）天生有跨迭代依赖，普通 parallel_for 直接写共享变量
 * 会数据竞争。TBB 用"分段局部累加 + 自相似合并"解决：递归对分成子区间各自求局部和，
 * 再把局部和两两合并。两种写法都演示：
 * - lambda：reduce(init, scan_lambda, combine_lambda)
 * - 经典 body：operator() 累加 + join 合并（oneTBB 2021+ 用单个 join；老 TBB 用
 *   reverse_join+assign），是理解归约机制的钥匙
 *
 * ⚠️ 反面教材：若用 parallel_for 让所有任务写同一个 `total += v[i]` → 数据竞争，
 *    结果错误。归约必须走 parallel_reduce（或每任务私有 + critical，但那样慢）。
 *
 * 编译指令：
 * g++ -O2 -std=c++17 parallel_reduce_sum.cpp -o parallel_reduce_sum $(pkg-config --cflags --libs tbb)
 *
 * 运行：
 * ./parallel_reduce_sum
 */

#include <oneapi/tbb.h>
#include <cstdio>
#include <vector>
#include <cmath>

#define N 10000000

// ---------- 经典 body 写法 ----------
struct SumBody {
    const std::vector<double>& v;
    double sum;
    SumBody(const std::vector<double>& vv) : v(vv), sum(0.0) {}
    // split：this 与 b 各持独立局部和（关键：局部和必须从 0 起，不能继承 b 的累加值）
    SumBody(SumBody& b, tbb::split) : v(b.v), sum(0.0) {}
    void operator()(const tbb::blocked_range<size_t>& r) {
        double local = 0.0;
        for (size_t i = r.begin(); i != r.end(); ++i) local += v[i];
        sum += local;
    }
    // oneTBB 2021+：合并两个子结果用单个 join()——把 b 的结果并入 this。
    // （老 TBB 4.x 是 reverse_join + assign 两个方法，迁移时注意改名。）
    void join(SumBody& b) { sum += b.sum; }
};

int main() {
    std::vector<double> v(N);
    double serial = 0.0;
    for (int i = 0; i < N; ++i) { v[i] = (i % 1000) * 0.5; serial += v[i]; }

    // ---------- 写法 A：lambda ----------
    double sum_lambda = tbb::parallel_reduce(
        tbb::blocked_range<int>(0, N, 10000), 0.0,
        [&](const tbb::blocked_range<int>& r, double partial) {
            for (int i = r.begin(); i != r.end(); ++i) partial += v[i];
            return partial;
        },
        [](double a, double b) { return a + b; });

    // ---------- 写法 B：经典 body ----------
    SumBody body(v);
    tbb::parallel_reduce(tbb::blocked_range<size_t>(0, N, 10000), body);
    double sum_body = body.sum;

    printf("串行和      = %.0f\n", serial);
    printf("reduce lambda = %.0f  (diff=%.3g)\n", sum_lambda, fabs(sum_lambda - serial));
    printf("reduce body   = %.0f  (diff=%.3g)\n", sum_body, fabs(sum_body - serial));

    // 求最大值：归约算子换成 std::max
    double vmax = tbb::parallel_reduce(
        tbb::blocked_range<int>(0, N, 10000), 0.0,
        [&](const tbb::blocked_range<int>& r, double cur) {
            for (int i = r.begin(); i != r.end(); ++i) if (v[i] > cur) cur = v[i];
            return cur;
        },
        [](double a, double b) { return a > b ? a : b; });
    printf("parallel_reduce 求最大值 = %.1f\n", vmax);

    return 0;
}

/**
 * 与 OpenMP reduction(+:sum) 的对照（第 8 课）：
 * - OpenMP：一条子句搞定，运行时自动私有副本 + 末尾合并。
 * - TBB：把"局部累加"和"合并"两个函数显式写出来（lambda 或 body 接口），
 *   灵活性更高（可自定义合并逻辑，如归约成向量、字符串拼接等）。
 *
 * 浮点归约的坑：合并顺序不唯一 → 浮点求和结果可能有末位差异（非确定性），
 * 对严格可复现有要求的场景要用定点/整数或 Kahan 求和。
 */
