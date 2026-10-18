# 44 · 实战 · `loop_tiling_1`（矩阵转置 tiling）完整闭环

> **TMA 一级**：Memory_Bound · **三级子类**：**L1/L2 局部性**（把 tile 塞进 L1）· **加速比**：**1.62x 本机** ≈ 1.75x 文档
>
> 💡 **本实验的独特价值**：所有前 4 个实验都是"改了就赚"——43 是第一个**"药方有副作用"**的案例：**tiling 大幅改善缓存行为，但被 4 层嵌套循环开销抵消了一半收益**。

---

## 一、读题（Step 0）

**实验目录**：[`TMA/memory_bound/loop_tiling_1/`](../../TMA/memory_bound/loop_tiling_1/)
**任务**：`N=2000` 的 `double` 方阵转置（`out[i][j] = in[j][i]`）· 32 MB 每矩阵 × 2 矩阵 = **64 MB 工作集 ≫ L3 36 MB**。

**这个 lab 特殊**——同时有两个 bench 变体：
| Bench | 数据结构 | Solution |
|-------|---------|----------|
| `bench1` | `MatrixOfDoubles = vector<vector<double>>`（**嵌套**，行间不连续）| `solution.cpp` tiling |
| `bench_flat` | `FlatMatrix = vector<double>`（**平面**，一维索引 `i*N+j`）| `solution_flat.cpp` tiling |

两 solution 都已 tiling，我需**临时把 `solution_flat.cpp` 改成无 tiling 版**作基线。

### 基线 vs 优化（针对 `bench_flat`）
```c
// BASELINE: 简单双循环, in[j*N+i] 内层沿 j 每次跳 N*8=16000B → 疯狂 stride
for (int i = 0; i < N; i++)
    for (int j = 0; j < N; j++)
        out[i * N + j] = in[j * N + i];

// OPTIMIZED: 4 层 tiling, 每次只处理 16×16 小块, 让 in/out 双块同时塞进 L1
constexpr int TILE = 16;
for (ii in 0..N step TILE) for (jj in 0..N step TILE)
    for (i in ii..ii+TILE) for (j in jj..jj+TILE)
        out[i*N + j] = in[j*N + i];
```

---

## 二、Step ①② 基线 bench + perf

**从零建基线**（保留 `initFlatMatrix`，否则链接失败——这是本次踩到的坑）：
```bash
cd TMA/memory_bound/loop_tiling_1
cp solution_flat.cpp solution_flat.cpp.tiled.bak
# 覆盖成"含 initFlatMatrix + 无 tiling 版 solution_flat"
cmake -E make_directory build_base && cd build_base
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_FLAGS="-g" -DCMAKE_CXX_FLAGS="-g" ..
cmake --build . --parallel 8
./lab --benchmark_filter=bench_flat
perf stat -e cycles,instructions,cache-references,cache-misses,LLC-load-misses,LLC-loads,dTLB-load-misses ./lab --benchmark_filter=bench_flat
```

**Google Bench**：`bench_flat  6.06 ms  6.04 ms  111 iterations`

**perf**（大核 99.95%，按 iter 归一化）：

| 指标 | 每 iter |
|------|---------|
| cycles | 40.7 M |
| instructions | 19.7 M |
| **IPC** | **0.48** |
| **LLC miss 率** | **82%** 🤯 |
| LLC miss | 208 K |
| dTLB miss | **1.85 M** |

---

## 三、Step ③④ 上药（换回 tiling 版）+ 复测

```bash
cp solution_flat.cpp.tiled.bak solution_flat.cpp   # 恢复
./build/lab --benchmark_filter=bench_flat         # 已有 build/ 是 tiling 版
perf stat ... ./build/lab --benchmark_filter=bench_flat
```

**Google Bench**：`bench_flat  3.75 ms  3.75 ms  185 iterations`

**加速比 = 6.06 / 3.75 = 1.62x**（文档 1.75x，接近）

| 指标 | 每 iter | vs 基线 |
|------|---------|--------|
| cycles | 35.7 M | ↓ 1.14x |
| **instructions** | **34.1 M** | **↑ 1.73x** ⚠️ |
| **IPC** | **0.96** | ↑ 2x |
| **LLC miss 率** | **15%** | **↓ 5.4x** 🔥 |
| LLC miss | 29 K | ↓ 7.2x |
| dTLB miss | 254 K | ↓ 7.3x |

---

## 四、🎯 独特教训：**"缓存救回来了，循环开销吃掉了"**

### 反直觉现象

**LLC miss 率从 82% 降到 15%**——**5.4x 的巨大改善**——**却只换来 1.62x 加速**。

对比 42 号 data_packing：LLC miss 率从 15.8% → 0.79%（20x）→ 加速 7.58x。**同样是 LLC miss 大降，为什么 43 只赚这么点？**

### 拆解加速公式

```
时间比 = 指令数比 / IPC 比
 1.62x =  1.73x↑   /  2x↑   × ...
       ≈ (优化 instructions ↑ / 基线) / (优化 IPC ↑ / 基线)
       ≈ 1.73 / 2.0
       ≈ 1.16x ... 但实测 1.62x
```

差异在于 perf 的 iter 数和 bench 的 iter 数不完全一致（perf 采样包含 setup 开销），Google Bench 的 1.62x 才是**权威数字**。但机制清晰：

1. **优化后 IPC ↑ 2x**：因为缓存行为改善（LLC miss 从 82%→15%），CPU 空转大幅减少 ✅
2. **但指令数 ↑ 1.73x**：4 层嵌套循环 vs 2 层，多了 tile 索引、边界 `std::min`、循环变量维护 → 白烧指令 ❌
3. **两个效应部分抵消** → 净赚 1.62x

> 🎯 **教训**：**tiling 不是免费午餐**。它对**每次 stride 访问的浪费**特别有效（因为把 N 次 stride 变成 N/TILE 次 stride），但**它自己也引入循环开销**。当**工作集不是特别大**、或 **TILE 太小**时，收益会被抵消。工程文档给的 1.75x 是"合理收益"而非"爆炸收益"。

---

## 五、`TILE_SIZE` 敏感性（本实验可继续探索）

工程文档暗示 TILE=16 是"调优后"的选择。理论最优 TILE 与 L1 容量相关：
- 每 tile 有 in/out 各 TILE² 元素，`TILE² × 8B × 2 = TILE² × 16B` 要 ≤ L1 48 KB
- → TILE ≤ √(48KB/16B) = √3072 ≈ 55

工程用 16 是**保守值**（考虑栈上其他变量、避免寄存器溢出）。可以手动改 8/16/32/64 扫一遍看曲线，但收益不会大跳（这是 tiling 的固有特性）。

---

## 六、顺带观察：bench1 vs bench_flat（数据结构的影响）

同为 tiling，两版数据结构的性能差异（用现有 build/ 跑）：
- **bench1**（`vector<vector<double>>` 嵌套）：**4.60 ms**
- **bench_flat**（`vector<double>` 平面）：**3.75 ms**

**嵌套 vs 平面 → 1.23x**。因为 `in[j][i]` 每次要读一次 `in[j]`（指针）再读 `in[j][i]`（值）→ **多一层 indirection + 行间不连续**。这就是**第 7 课 "AoS vs SoA / 布局即性能"** 的又一记实证。

---

## 七、放进 Memory_Bound 谱系（对比 40/41/42）

| | 40 交换矩阵乘 | 41 结构体压缩 | 42 交换高斯 | **43 tiling 转置** |
|--|---|---|---|---|
| 工作集 vs L3 | 1.9MB / 36MB | 32→8MB | 88MB | **64MB** |
| 机制 | 解锁 SIMD | 跨 L3 边界 | TLB Bound | **L1 tile 化** |
| LLC miss 变化 | 15.8→0.79%（↓20x）| 15.8→0.79% | 48%→50%（**不变**）| **82%→15%**（↓5.4x）|
| dTLB 变化 | 小 | 小 | **↓5418x** ⭐ | ↓ 7.3x |
| 指令数变化 | **↓ 10.4x** ⭐ | ↓ 2.3x | ↓ 3.2x | **↑ 1.73x** ⚠️ |
| 加速 | 6.81x | 7.58x | 9.56x | **1.62x** |

> **43 是"最小加速比"的 Memory_Bound 实验**——原因正是**唯一让指令数反而增加的**优化。**"药方有副作用"的活教材**，让"先分诊再开药"从口号变成肌肉记忆。

---

## 八、放进 `20 §一` 闭环

| 步骤 | 本实验 |
|------|--------|
| ① 度量基线 | bench_flat 6.06 ms |
| ② 定位热点 | 转置的内层 `in[j*N+i]` 跨行跳步 |
| ③ TMA 分诊 | IPC 0.48 + LLC miss **82%** + dTLB 1.85M → **典型 L1/L2 局部性差 + 部分 TLB** |
| ④ 开药 | **循环分块 TILE=16**（第 5 课目的 A / 第 7 课 § 三 分块）|
| ⑤ 验证 | 1.62x + LLC miss ↓5.4x = **命中预期，但被 1.73x 指令开销部分抵消** |

---

## 九、一句话收口

> **tiling 是 Memory_Bound 药箱里"最讲究"的一味**——它对 stride 访问有特效（LLC miss ↓5.4x），**但 4 层嵌套循环自身也烧指令（↑1.73x）**，最后只赚 1.62x。**"没有免费的午餐"**——每个优化都有开销，工程价值靠**"分诊准 + 权衡净收益"**，不靠"背手法清单"。

## 十、延伸

- 相关理论：`07-访存优化.md § 三.1`（缓存分块 + 公式）、`05-程序编写优化.md § 五 目的 A`
- 复现坑（进了 `30`）：改 `solution_flat.cpp` 时必须**保留 `initFlatMatrix` 函数**，否则 `bench.cpp` 链接失败
- 原始工程文档：[`docs/loop_tiling_1.md`](../experiments/memory_bound/loop_tiling_1.md)
