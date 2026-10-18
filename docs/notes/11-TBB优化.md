# 11 · TBB 程序优化

> **立意**：第 8 课 OpenMP 教你"**给循环贴个标签就并行**"（面向线程、fork-join）。TBB 把抽象再抬一层——**面向任务**：你只管"怎么把活切碎、每块干什么"，**"用几个线程、谁先谁后、忙不过来怎么匀"全交给运行时的工作窃取引擎**。于是 OpenMP 里要靠 `schedule` / 并行区扩张 / 手写 reduction 精心伺候的那些坑，TBB 大多**默认就帮你扛了**。代价：它是纯 C++ 库、心智模型从"线程"切到"任务 + 递归对分"。

---

## 一、TBB 是什么：不贴标签，直接调库

Linux 装 `libtbb-dev`（本机另有 Intel oneAPI TBB，`pkg-config tbb` 指向它）。C++17，`#include <oneapi/tbb.h>`。

```cpp
tbb::parallel_for(tbb::blocked_range<int>(0, N), [&](const tbb::blocked_range<int>& r) {
    for (int i = r.begin(); i != r.end(); ++i) C[i] = A[i] + B[i];
});
```
编译运行：`g++ -O2 -std=c++17 app.cpp -o app $(pkg-config --cflags --libs tbb) && ./app`

**Fork-Join（OpenMP） vs 递归对分 + 工作窃取（TBB）**：
```
OpenMP: 主线程 ─ Fork(建 T 个线程) → 各跑一段 → Join(barrier)      ← 每次进/出并行区都付代价
TBB   : [0,N) →对分→ [0,N/2)+[N/2,N) →再对分→ … 直到 grainsize      ← 任务树
        空闲线程从忙线程队列"偷"未执行区间(work-stealing) → 天然负载均衡
```
> 👉 **一句话**：OpenMP 你操心和"线程"的关系，TBB 让你只操心"任务怎么切"。

**入门代码**：[`tbb_hello.cpp`](../../basis/TBB/Basics/tbb_hello.cpp) → [`parallel_for_basics.cpp`](../../basis/TBB/Basics/parallel_for_basics.cpp) → [`matrix_multiply_parallel.cpp`](../../basis/TBB/Basics/matrix_multiply_parallel.cpp)

---

## 二、头号对照：变量作用域（闭包捕获，无需标注）

第 8 课的头号坑 `shared/private/reduction`，在 TBB 里**消失了一半**：

| OpenMP | TBB |
|--------|-----|
| `shared(x)` | 外部变量被 `[&]` 捕获 → 天然共享 |
| `private(x)` | 循环体/lambda 内的局部变量 → 天然私有 |
| `reduction(+:sum)` | 用 `tbb::parallel_reduce`（见下）|

**剩下没消失的是归约**：`sum += v[i]` 这类跨迭代依赖，直接写共享变量仍是数据竞争。TBB 的解法是 `parallel_reduce`——把"每块局部累加"和"合并"两个函数显式写出：
```cpp
double sum = tbb::parallel_reduce(
    tbb::blocked_range<int>(0, N), 0.0,
    [&](const tbb::blocked_range<int>& r, double partial) {          // 局部累加
        for (int i = r.begin(); i != r.end(); ++i) partial += v[i];
        return partial; },
    [](double a, double b) { return a + b; });                       // 合并
```
比 OpenMP 一条子句啰嗦，但**合并逻辑可自定义**（归约成向量、直方图、字符串都行）。
代码：[`parallel_reduce_sum.cpp`](../../basis/TBB/ParallelAlgorithms/parallel_reduce_sum.cpp)、[`parallel_scan_prefix.cpp`](../../basis/TBB/ParallelAlgorithms/parallel_scan_prefix.cpp)（前缀和，同 MPI_Scan/`omp scan` 母题）。

> ⚠️ **oneTBB 2021+ API 迁移坑**：经典 body 归约用单个 `join(Body&)`，**不再是**老 TBB 的 `reverse_join`+`assign`；`parallel_scan` 的合并 lambda **必须返回**合并值。写错直接编译失败。

---

## 三、⭐ 灵魂：递归对分 + 分区器（把 `schedule` 做成参数）

工作窃取是 TBB 的立身之本，OpenMP 里"负载不均靠 `schedule(dynamic)`"的功课，TBB 用**分区器（partitioner）**这个可选参数吸收：

| 分区器 | = OpenMP 里的 | 何时用 |
|--------|--------------|--------|
| `auto`（默认）| `schedule(dynamic)`+自适应 | **无脑首选**：递归对分 + 窃取 + 支持嵌套 |
| `simple` | `static`（但会先切细）| 迭代均匀、单层，省细分开销 |
| `static` | `static`（不窃取）| 块耗时极均衡才划算，不均时最惨 |
| `affinity` | `OMP_PROC_BIND`+块记忆 | 多轮迭代、数据位置稳定（时间步/迭代求解）|

**实测**（[`work_stealing.cpp`](../../basis/TBB/TaskGraph/work_stealing.cpp)，造一个"越靠后越贵"的循环）：`auto` 比 `static` 快 **~1.8–2.3x**——负载越不均差距越大，正是窃取的功劳。

代码：[`auto_partitioner_nested.cpp`](../../basis/TBB/LoadBalancing/auto_partitioner_nested.cpp)、[`simple_partitioner.cpp`](../../basis/TBB/LoadBalancing/simple_partitioner.cpp)、[`affinity_partitioner.cpp`](../../basis/TBB/LoadBalancing/affinity_partitioner.cpp)

---

## 四、动态任务：`task_group`（并行里再并行，安全）

OpenMP 的 fork-join 是"固定并行区"；TBB 能在**运行时递归地派生任务**——任务树有多少节点就榨多少并行：

```cpp
long fib(long n) {
    if (n < 2) return n;
    if (n < CUTOFF) return fib_serial(n);      // ← 关键：串行 cutoff
    long f1, f2; tbb::task_group g;
    g.run([&]{ f1 = fib(n-1); });              // 异步派生左子树
    f2 = fib(n-2);                             // 当前线程做右子树
    g.wait();                                  // 汇合
    return f1 + f2;
}
```
**实测**（[`task_group_fibonacci.cpp`](../../basis/TBB/TaskGraph/task_group_fibonacci.cpp)）：fib(35) 串行 0.011s → 并行 0.002s ≈ **7x**。

> ⚠️ **血泪坑（本机实测复现）**：**去掉 cutoff 会直接段错误**。无脑对每个 n 派生子任务，fib(30) 就炸出上千万任务 → 栈/内存溢出。**任务并行度必须有下限**。这也是 TBB 相对 OpenMP 的隐藏优势：**嵌套并行天然安全**（内层不另起一批线程，复用同一窃取池），而 OpenMP 默认禁用嵌套、要 `OMP_NESTED`。

其它并行算法：[`parallel_invoke_sort.cpp`](../../basis/TBB/ParallelAlgorithms/parallel_invoke_sort.cpp)（独立任务 + 并行排序，`parallel_sort` 5M int 实测 **6x**）、[`parallel_pipeline_filter.cpp`](../../basis/TBB/ParallelAlgorithms/parallel_pipeline_filter.cpp)（三阶段流水，实测 **~10x**，对应第 21 课母题①"掩盖延迟"）。

---

## 五、伪共享（第 7/8 课元凶的 TBB 三档阶梯）

同一课的知识，TBB 换套工具：

```
❌ false_sharing_analysis.cpp     紧凑 result[tid]：槽位共享缓存行 → RFO 乒乓
✅ cache_aligned_counter.cpp      tbb::cache_aligned_allocator + 每槽独占一行(stride=8)
✅✅ reduction_avoid_false_sharing.cpp  parallel_reduce：压根不构造共享数组（首选）
```
> 与第 8 课结论一致：**优先 reduction**（简洁、省内存、顺带消争用）；只有"每线程结果要分别保留"时才用 padding/cache_aligned。

代码：[`FalseSharing/`](../../basis/TBB/FalseSharing/)；对照工程实验 [`TMA/memory_bound/false_sharing_1/`](../../TMA/memory_bound/false_sharing_1/)（`alignas(64)` 消伪共享）。

---

## 六、工具箱速览（并发容器 / 流图 / 分配器）

- **并发容器**（[`ConcurrentContainers/`](../../basis/TBB/ConcurrentContainers/)）：`concurrent_vector`（无锁追加）、`concurrent_hash_map`（按桶细粒度锁，处理"多线程命中同一 key"的**真共享**）、`concurrent_queue`（工作池）。**选型提醒**：纯离线归约用 `parallel_reduce` 常比上并发容器更快——容器只在**需在线读写/动态 key**时用。
- **flow_graph**（[`graph_pipeline.cpp`](../../basis/TBB/FlowGraph/graph_pipeline.cpp)）：节点+边的 DAG 表达复杂依赖（分支、扇入扇出、反馈），自动流水重叠。实测 **2.1–2.4x**（工作量大才划算，小负载图开销反而拖慢）。
- **scalable_allocator**（[`scalable_allocator.cpp`](../../basis/TBB/MemoryAllocation/scalable_allocator.cpp)）：替换默认 malloc 的全局锁瓶颈，多线程高频小分配才见效（本负载仅 1.06x）。

---

## 七、本机实测 · 加速比曲线（N=1200 稠密矩阵乘，`-O3 -march=native`，3 次取最小）

| 线程 | 实测(s) | 加速比(vs 串行 0.110s) |
|---|---|---|
| 2 | 0.061 | 1.81x |
| 4 | 0.036 | 3.11x |
| 8 | 0.019 | 5.70x |
| 12 | 0.017 | 6.4x |
| 16 | 0.012 | 8.98x |
| **24** | 0.011 | **~10x** |

**读曲线**（呼应第 8 课实验⑦）：前段近线性，后段撞 **Amdahl 串行段 + 内存带宽 + 大小核混合架构** 三重天花板，封顶 ~10x。`auto` 分区器**无需手动 `schedule`** 就跑出这条曲线——这就是"任务框架"省心的地方。

**诚实的未复现项**（`30 测量纪律手册` 的活标本）：
- 伪共享微基准只有 ~1.3ms，run1 差 5x、run2 三者持平 → **规模太小 + 调度噪声掩盖效应**，别急着下结论。
- `affinity` / `simple-vs-auto` / `scalable_allocator` 在各自的带宽/访存瓶颈负载上**看不出差别**——工具只在匹配的瓶颈上才有效。

---

## 八、TBB vs OpenMP：何时选谁（本课落点）

| 维度 | OpenMP | TBB |
|------|--------|-----|
| 抽象 | 线程（fork-join）| **任务**（递归对分 + 窃取）|
| 上手 | 贴 `#pragma` 最快 | C++ 模板/lambda，稍陡 |
| 负载均衡 | 手动 `schedule` | **默认 auto 自动搞定** |
| 嵌套并行 | 默认禁用 | **天然安全** |
| 不规则/递归任务 | 弱 | **强**（task_group / flow_graph）|
| 语言 | C/C++/Fortran | **仅 C++** |
| 移植性 | 编译器内建，无链接 | 需装库、链接 `-ltbb` |

> **一句话选型**：规整的稠密循环并行，OpenMP 一行 pragma 最省事；一旦涉及**递归分治、动态任务、复杂依赖(DAG)、嵌套并行**，TBB 的工作窃取框架是更大的杀器。

---

## 九、一句话收口

> **TBB = "把并行写成对任务的递归切分，剩下的交给工作窃取"**。头号对照是**变量作用域靠闭包捕获天然正确**、**归约交给 `parallel_reduce`**；**灵魂是分区器 + 工作窃取**（默认 `auto` 就把负载不均有嵌套全包了）；**杀手锏是动态任务 `task_group` 与 `flow_graph`**（但递归任务**必须加 cutoff** 否则段错误）；伪共享/带宽等第 7/8 课的老病，TBB 换个工具名（cache_aligned/reduce）照样要治。**加速比同样撞 Amdahl + 带宽 + 大小核天花板**。

## 十、延伸阅读

- 示例代码：[`basis/TBB/`](../../basis/TBB/)（含 `README.md` 编译/实测汇总）
- 姊妹范式理论长文：[`../theory/openmp_optimization.md`](../theory/openmp_optimization.md)、[`../theory/cuda_optimization.md`](../theory/cuda_optimization.md)、[`../theory/mpi_optimization.md`](../theory/mpi_optimization.md)
- 相关实验：[`../experiments/memory_bound/false_sharing.md`](../experiments/memory_bound/false_sharing.md)
- 承接：**`21-并行优化全景复盘.md`**（把 OpenMP/CUDA/MPI/TBB 四个模型缝成一张图，看"四大母题"如何跨模型同构）
