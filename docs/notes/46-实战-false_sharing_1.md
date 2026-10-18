# 46 · 实战 · `false_sharing_1`（多线程 alignas 消伪共享）

> **TMA 一级**：Memory_Bound · **三级子类**：**Store Bound / 跨核缓存行争用** · **加速比**：工程 **15.8x**，**本机仅 ~0%（复现失败）**
>
> ⚠️ **本实验的独特价值不在加速比、而在"负结果"**：一次罕见的、**在同一代码上无法复现文档数字**的案例，把 `30` 元教训 3（"加速比强依赖硬件与负载"）**用反面证据钉死**。

---

## 一、读题（Step 0）

**实验目录**：[`TMA/memory_bound/false_sharing_1/`](../../TMA/memory_bound/false_sharing_1/)
**核心代码**：OpenMP 并行 for 累加，每线程独占一个 `std::atomic<uint32_t>` 累加器：
```cpp
#define CACHELINE_ALIGN alignas(64)                // 关键: 每累加器独占 64B 缓存行
struct CACHELINE_ALIGN Accumulator {
    std::atomic<uint32_t> value = 0;
};
std::vector<Accumulator> accumulators(thread_count);

#pragma omp parallel num_threads(thread_count) ...
    int target_index = omp_get_thread_num();
    auto& target = accumulators[target_index];
    #pragma omp for
    for (int i = 0; i < data.size(); i++) {
        ...
        target.value += item % 13;                 // 每线程写自己的 atomic
    }
```

**baseline vs optimized**：
- 优化：`alignas(64)` → `sizeof(Accumulator) = 64B` → 各线程缓存行**独占**
- 基线：无 alignas → `sizeof(Accumulator) = 4B` → 16 线程挤在**同一** 64B 缓存行

**bench.cpp** 对 **thread_count = 1..24** 依次跑一遍（Google Bench `UseRealTime`）。

---

## 二、工程文档基线（他机数据）

| 指标 | 优化前 | 优化后 | 变化 |
|------|-------|-------|------|
| **real_time** | **324 ms** | **20.5 ms** | **15.8x** |
| CPU time | 163 ms | 15.6 ms | 持平 |
| Core Bound | 51.4% | 49.4% | -2.0% |
| Memory Bound | 28.2% | 31.0% | +2.8% |

**关键机制**（工程文档 § 分析）：
- **real 324 >> CPU 163** → 线程花在"等 MESI 缓存行独占权"上的时间达 161ms（**等待 = real − CPU**）
- 优化后 real(20.5) ≈ CPU(15.6) → 争用几乎消失

---

## 三、本机实测：**未能复现加速**

**基线**（把 `alignas(64)` 换成 `__attribute__((packed))`）：
```
bench1/real_time   97.6 ms   15.5 ms   7 iterations
cpu_core/cycles 4.25 B, instructions 3.83 B, IPC 0.90, LLC miss 40 K
```

**优化后**（当前 `alignas(64)` 版本）：
```
bench1/real_time   99.9 ms   15.6 ms   7 iterations
cpu_core/cycles 4.33 B, instructions 3.84 B, IPC 0.89, LLC miss 41 K
```

**加速比 = 97.6 / 99.9 = 0.98x**（**反而略慢**，几乎无差）。

**关键观察**：两版 `real ≈ 100ms`、`CPU ≈ 15.5ms`——**real/CPU = 6.4x 差距**在**优化版也存在**！而工程文档说优化后 `real ≈ CPU`（20.5 vs 15.6）。这说明本机有**别的等待源**在**同时压两版**。

---

## 四、🎯 为什么本机复现失败？三个可能原因（诚实分析）

1. **`std::atomic<uint32_t>::+=` 走 `lock xadd`**——即使在同一缓存行，硬件层面 atomic 已经是"每 cache line 序列化"；跨线程 ping-pong 的代价**可能已被 atomic 自身的高开销吞掉**。工程基线可能测的是**普通 uint32_t 累加**（无 atomic）+ `omp for`，那时 false sharing 效应才纯。

2. **调度器没让线程真正铺到 24 核**：本机是 P+E 混合架构（12 P + 12 E？），`OMP_PROC_BIND=spread` + `taskset 0-23` 组合下，OpenMP 可能把高编号线程压到 E 核或同核 → 争用没跨到 24 个 MESI 域。

3. **real >> CPU 的 6.4x 差异**（两版都有）**指向别的等待源**——最可能：
   - `for (auto thread_count : threads)` 从 1 到 24 每档都要 **fork-join 一次**，OpenMP 团队启动/收尾开销 = real 时间的大头
   - 这个开销和 false sharing 无关 → 掩盖了差异

> 💡 **教训**：**优化只在瓶颈真存在时才有收益**——`30` 手册元教训 3 的最强案例。当"别的开销"（fork-join）主导时，你**看不出**"你试图消除的那个开销"（false sharing）有多大改善。

---

## 五、如何修正这个实验（本机可复现版思路）

要看到 false sharing 效应，需要**隔离出"每线程 atomic++ 在紧挨着的地址上"**这一因素。三种可选修法（下次可试）：

1. **只测高 thread_count**：把 bench 里的 `for (thread_count : 1..24)` 改成 `for (24)` → fork-join 开销稀释掉。
2. **去掉 atomic**：改成普通 `uint32_t` + `#pragma omp for reduction` 或者干脆手写各线程私有，避免 `lock xadd` 吞掉争用信号。
3. **绑核 & 打印 tid→cpu**：`OMP_DISPLAY_ENV=true` 或 `numactl --hardware` 确认真正跨核分布。

---

## 六、Memory_Bound 药箱谱系收官（40–45 全景）

| # | 实验 | 本机加速 | 工程文档 | 复现状况 |
|---|------|---------|---------|---------|
| 40 | loop_interchange_1 | **6.81x** | 6.9x | ✅ 完美 |
| 41 | data_packing | **7.58x** | 7.5x | ✅ 完美 |
| 42 | loop_interchange_2 | **9.56x** | 9.7x | ✅ 完美 |
| 43 | loop_tiling_1 | **1.62x** | 1.75x | ✅ 接近 |
| 44 | swmem_prefetch_1 | **3.13x** | 3.1x | ✅ 完美 |
| **45** | **false_sharing_1** | **0.98x（复现失败）** | **15.8x** | ❌ **环境不匹配** |

> **45 是整套实验里唯一"复现失败"的一个**——但它**恰恰是 `30` 手册最鲜活的教材**：
> - **文档数字不能盲信**
> - **优化前提**是"该瓶颈真的存在且未被其他开销掩盖"
> - **复现失败的原因值得研究**——不是"我算错了"，是"环境差异 + 瓶颈被别的东西吃掉"

---

## 七、放进 `20 §一` 闭环（本实验的特殊闭环）

| 步骤 | 本实验 |
|------|--------|
| ① 度量基线 | 本机 baseline 97.6ms |
| ② 定位热点 | 名义上：跨核 atomic 争用 |
| ③ TMA 分诊 | real/CPU = 6.4x 但两版都这样 → 说明**争用不是本机瓶颈** |
| ④ 开药 | `alignas(64)` 每累加器独占缓存行 |
| ⑤ 验证 | **失败**：加速 0.98x，本机没测出效应 |
| ⑥ **元验证** | **对照工程文档 → 归因到环境/其他开销 → 记入 `30` 手册**（这一步是"负结果"的关键附加价值） |

---

## 八、一句话收口

> **不是所有实验都能跨环境复现**——46 号是"复现失败"的活标本。工程文档的 15.8x 来自**跨核 atomic 真争用**的场景；本机因为 `lock xadd` 吞掉信号 + fork-join 开销主导，两版差异 < 3%。**这不代表 alignas(64) 没用**——它代表**"优化只对当前环境的真实瓶颈有效"**。把 45 和 `30` 元教训 3 一起读，比单纯看 15.8x 学到的多得多。

## 九、延伸

- 相关理论：`07-访存优化.md § 三.3`（伪共享原理）、`08-OpenMP优化.md § 五 实验⑥`（第一次翻车的伪共享小实验）、`21-并行优化全景复盘.md § 三.母题 ③`（消争用）
- 相关工具：`perf c2c`（本机未测，可下次跑）
- 复现方法改进（本实验暴露的）：bench 应**分别报各 thread_count 的时间**，而不是 1..24 求和；或固定 thread_count=24 只看高并发
