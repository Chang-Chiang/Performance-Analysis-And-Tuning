# 45 · 实战 · `swmem_prefetch_1`（哈希表随机查找 + 软件预取）完整闭环

> **TMA 一级**：Memory_Bound · **三级子类**：**DRAM Bound（延迟隐藏）** · **加速比**：**3.13x 本机** ≈ 3.1x 文档
>
> 💡 **本实验的独特价值**：前 4 个 Memory_Bound 实验都在"**减少 miss**"（40/41/42/43），44 是**第一个"不减少 miss 但依然提速"**——**预取的本质是"提前发出 miss、让它和计算重叠"，不是"消除 miss"**。

---

## 一、读题（Step 0）

**实验目录**：[`TMA/memory_bound/swmem_prefetch_1/`](../../TMA/memory_bound/swmem_prefetch_1/)

**规模（决定瓶颈）**：
```
HASH_MAP_SIZE     = 32M - 5 buckets × int(4B) ≈ 128 MB   ≫ L3 36MB
NUMBER_OF_LOOKUPS = 1M 次随机 find (uniform 分布)
```
`find(val)` = 一次哈希算 bucket → 一次随机 DRAM 访问。

**基线 vs 优化**：
```c
// BASELINE: 无预取, 每次 find 直接暴露 ~300ns DRAM 延迟
for (i in 0..lookups.size())
    if (hash_map->find(lookups[i])) result += getSumOfDigits(lookups[i]);

// OPTIMIZED: 提前 16 步 prefetch 未来要用的 bucket
constexpr int look_ahead = 16;
for (i in 0..size-16) {
    if (hash_map->find(lookups[i])) result += getSumOfDigits(lookups[i]);
    hash_map->prefetchForVal(lookups[i + 16]);   // 内部 __builtin_prefetch(&m_vector[bucket])
}
for (i in size-16..size) ...                      // 尾巴单独处理
```

**为什么这里预取有效、其他地方无效**（第 7 课铁律的实证）：
- 每次 find 的地址 = `val % N_Buckets`，**硬件预取器看不到规律**（不像顺序访问那样可以预测下一字节）→ 硬件救不了
- 但 **软件层知道 `lookups[i+16]` 未来一定会访问** → 提前 16 步启动 DRAM 加载，等真的用到时数据已在 L1

---

## 二、Step ①② 基线 bench + perf

```bash
cd TMA/memory_bound/swmem_prefetch_1
cp solution.cpp solution.cpp.opt.bak
# 覆盖 solution.cpp 为"无 prefetch 版"
cmake -E make_directory build_base && cd build_base
cmake -DCMAKE_BUILD_TYPE=Release ... && cmake --build . --parallel 8
taskset -c 0 ./lab
```

**Google Bench**：`bench1  49.1 ms  49.1 ms  14 iterations`

**perf**（大核 100%）：

| 指标 | 合计 |
|------|------|
| cycles | 41.90 B |
| instructions | 18.23 B |
| **IPC** | **0.44** |
| cache-misses (L1) / refs | 281M / 300M = **93.7%** |
| **LLC-load-misses / loads** | 120M / 126.5M = **94.9%** |
| dTLB-load-misses | 121.3 M |

---

## 三、Step ③④ 上药（预取 ahead=16）+ 复测

```bash
cp solution.cpp.opt.bak solution.cpp
taskset -c 0 ./build/lab
perf stat ... ./build/lab
```

**Google Bench**：`bench1  15.7 ms  15.7 ms  45 iterations`

**加速比 = 49.1 / 15.7 = 3.13x** ✅

**perf**：

| 指标 | 合计 | vs 基线 |
|------|------|---------|
| cycles | 39.58 B | 略降 |
| instructions | 21.47 B | ↑ 1.18x（多 prefetch 调用）|
| **IPC** | **0.54** | 略升 |
| cache-misses / refs | 287M / 317M = 90.7% | 略降 |
| **LLC miss 率** | **94.7%** | **几乎不变** 🎯 |
| **dTLB-load-misses** | **151.9 M** | **反而增加 25%** |

---

## 四、🎯 核心洞察：**"miss 数几乎没变，却快了 3.13x"**

这是本实验**最反直觉、最有教育意义**的数字：

| 观察 | 通常直觉 | 实际 |
|------|---------|------|
| LLC miss 率 | 应该大降 | **94.9% → 94.7%（不变）** |
| dTLB miss | 应该降或不变 | **反而多 25%**（prefetch 触发更多页翻译） |
| instructions | 应该少 | **多 18%**（每次循环多一条 prefetch 指令） |
| **总时间** | 应该会变差 | **↓ 3.13x** ⭐ |

### 预取到底做了什么？

预取**不减少 miss 数量**——工作集 128MB ≫ L3 36MB，DRAM 访问是物理宿命。它做的是：**把 miss 的发生时机往前挪**，让**内存访问的 ~300ns 延迟和后续几十次 find 的计算重叠起来**。

```
无预取:   find(i) → [DRAM 等 300ns] → find(i+1) → [DRAM 等 300ns] → ...
                        ↑ CPU 干等

预取:     prefetch(i+16) → [DRAM 加载 i+16 到 L1, 300ns 中...]
              find(i) → [DRAM 等 300ns] → prefetch(i+17)
              find(i+1) → [DRAM 等 300ns] → prefetch(i+18)
              ... 16 步之后, 当 find(i+16) 真发生时, 数据已在 L1 → 命中!
```

**关键参数 look_ahead=16**：正好覆盖 "DRAM 300ns ÷ 单次 find ~20ns = 15 步" 的距离。太短预取不到、太长浪费带宽 + 撞 L1 容量。

> 📚 **第 7 课铁律的完整闭环**：
> - "**顺序访问不要手动预取**"——硬件预取器早就搞定了
> - "**随机访问 + 未来访问可预测 → 软件预取才有戏**"——本实验完美验证：`lookups` 数组**内容已知**（1M 个 int 存在内存里），未来要找什么完全可预测；但**分布是 uniform 随机**（每个 bucket 独立），**硬件预取器无法从 stride 推断** → **只有软件预取救得了**。

---

## 五、放进 `20 §一` 闭环

| 步骤 | 本实验 |
|------|--------|
| ① 度量基线 | bench 49.1 ms |
| ② 定位热点 | `hash_map->find()` 里的 `m_vector[bucket]` 一次 DRAM 访问 |
| ③ TMA 分诊 | IPC 0.44 + LLC miss 94.9% → **DRAM Bound**（工作集远超 L3）|
| ④ 开药 | **`__builtin_prefetch` 提前 16 步触发未来 bucket 的加载** |
| ⑤ 验证 | 3.13x + **miss 数不变 → 收益 100% 来自"延迟隐藏"** ✅ |

---

## 六、Memory_Bound 药箱谱系再扩（40–44 全景）

| # | 实验 | 加速 | 核心机制 | **miss 数变化** |
|---|------|------|---------|---------------|
| 40 | loop_interchange_1 | 6.81x | 解锁 SIMD | 指令数 ↓10x |
| 41 | data_packing | 7.58x | 缩到 L3 内 | LLC miss **↓20x**（真减少）|
| 42 | loop_interchange_2 | 9.56x | 减少跨页 stride | dTLB miss **↓5418x**（真减少）|
| 43 | loop_tiling_1 | 1.62x | tile 塞 L1 | LLC miss ↓5.4x **但指令数 ↑** |
| **44** | **swmem_prefetch_1** | **3.13x** | **提前 miss、让 miss 藏进计算** | **miss 数几乎不变** 🎯 |

> **45 号补全了 Memory_Bound 药箱的第四种机制**——原来"减 miss"（41/42）和"增开销换减 miss"（43）之外，还有第三种"**不减少 miss、只把它挪到更早发生**"（44）。这一味药在**硬件预取猜不到、但软件层可预测**的场景独占鳌头（链表遍历、图 CSR/哈希查找、稀疏矩阵）。

---

## 七、一句话收口

> **预取 ≠ 减少 miss；预取 = 提前 miss 让它和计算重叠**。这就是"**软件预取只用在硬件猜不到的随机访问**"（第 7 课铁律）的物理依据——本实验用 **miss 数不变、时间却快 3.13x** 直接把它量化了。**look_ahead=16 ≈ DRAM 延迟 ÷ 单次操作时间**是选择预取距离的黄金公式。

## 八、延伸

- 相关理论：`07-访存优化.md § 三.4`（预取）、`01 § 三`（Roofline 里的"延迟隐藏"）
- 相关实验：`swmem_prefetch_1` 官方目录，工程 `docs/swmem_prefetch_1.md`
- 复现坑（`30` 手册）：用 bash heredoc 写 C++ 时 `!` 会被 bwrap 转义；改用 `while (n)` 而不是 `while (n != 0)`
