# 48 · 实战 · `vectorization_2`（Internet Checksum 消依赖链）完整闭环

> **TMA 一级**：**Core_Bound** · **优化技术**：**拓宽累加器消除循环依赖链 → 解锁 SIMD** · **加速比**：**21.0x 本机** ≈ 20.9x 文档
>
> ⭐ **Core_Bound 药箱的"镇箱之宝"**——和 47 号"只摆 SoA 不消依赖 = 2.1x"形成最鲜明对照，证明**"消依赖 > 摆布局"一个数量级**。

---

## 一、读题（Step 0）

**实验目录**：[`TMA/core_bound/vectorization_2/`](../../TMA/core_bound/vectorization_2/)
**任务**：算 64K 个 `uint16_t` 的 Internet Checksum（RFC 1071）——本质是**带模 2¹⁶-1 回绕的加法归约**。

### 基线（真依赖链）
```cpp
uint16_t checksum(const Blob &blob) {
  uint16_t acc = 0;
  for (const auto value : blob) {
    acc += value;
    acc += acc < value;      // ← 进位检查：新 acc 溢出则再加 1
  }
  return acc;
}
```
**为什么这行代码毁了向量化**：`acc < value` 判定**依赖本次加完的 acc**，而新 acc **又是下一次迭代的输入** → **每步串行**，编译器不能把 8 个 lane 并行加。

```
迭代依赖链（第 6 课"真依赖"最纯粹案例）:
  acc[0] = 0 + v0 + carry(acc[0] < v0)
  acc[1] = acc[0] + v1 + carry(acc[1] < v1)   ← 必须等 acc[0] 出来
  acc[2] = acc[1] + v2 + carry(...)          ← 必须等 acc[1]
  ...  65536 步, 完全串行
```

### 优化（拓宽 + 末尾折回）
```cpp
uint16_t checksum(const Blob &blob) {
  uint32_t acc = 0;                      // 拓宽到 32 位
  for (const auto value : blob) {
    acc += value;                        // ← 只剩加法, 无进位检查, 无依赖链!
  }
  // 循环结束后一次性折回 16 位 (数学上等价于反复 add-carry)
  auto high = acc >> 16, low = acc & 0xFFFFu;
  acc = low + high;
  high = acc >> 16; low = acc & 0xFFFFu;
  acc = low + high;
  return static_cast<uint16_t>(acc);
}
```

**关键洞察**：
- **32 位足够容纳 64K × 65535 不溢出**（`64K × 65535 < 2^32`）→ 循环内**不需要进位**
- 归约结束时**一次性折回**：把 32 位加和的高 16 位折回低 16 位 → **数学上等价于反复 add-carry**
- **循环内只剩 `acc += value`**：这是**关联**操作（顺序无关）→ 编译器放心 SIMD

---

## 二、Step ①② 基线 bench + perf

```bash
cd TMA/core_bound/vectorization_2
# 覆盖 solution.cpp 为上面"基线"版
cmake -E make_directory build_base && cd build_base
cmake -DCMAKE_BUILD_TYPE=Release ... && cmake --build . --parallel 8
taskset -c 0 ./lab
perf stat -e cycles,instructions,cache-misses,branch-misses taskset -c 0 ./lab
clang++ -O3 -march=native -Rpass-missed=loop-vectorize -c solution.cpp -o /dev/null   # ⭐
```

**Google Bench**：`bench1  24.8 us  24.8 us  28280 iterations`

| 指标 | 基线合计 |
|------|---------|
| cycles (cpu_core) | 5.29 B |
| instructions | 13.02 B |
| **IPC** | **2.46** |
| cache-misses | 101 K（工作集 128 KB 全在 L2） |

**⭐ `-Rpass-missed` 铁证**：
```
solution.cpp:4:3: remark: loop not vectorized [-Rpass-missed=loop-vectorize]
```
编译器坦白："**我不敢向量化**"——依赖链是硬伤。

---

## 三、Step ③④ 上药（消依赖）+ 复测

**Google Bench**：`bench1  1.18 us  1.18 us  592735 iterations`

**加速比 = 24.8 / 1.18 = 21.0x** ✅

| 指标 | 优化合计 | vs 基线 |
|------|---------|--------|
| cycles | 4.51 B | ↓ 1.17x |
| instructions | 26.14 B | ↑ 2.0x（多 iters，因为快）|
| **IPC** | **5.80** | **↑ 2.4x** ⭐ |
| cache-misses | 95 K | 差不多（都在 L2） |
| branch-misses | 770 K | 稍多但相对总指令可忽略 |

**⭐ `-Rpass` 铁证**：
```
solution.cpp:7:3: remark: vectorized loop (vectorization width: 8, interleaved count: 4) [-Rpass=loop-vectorize]
```
**width 8** = AVX2 一次 8 个 `uint32`（8×4B = 256 位）→ **8 路并行加法**。

**每 iter 归一化对比**：
- 基线：13.02B / 28280 = **460K 指令/iter**，IPC 2.46 → 时间 24.8μs
- 优化：26.14B / 592735 = **44K 指令/iter**，IPC 5.80 → 时间 1.18μs

指令数 ↓10.5x（SIMD 8 lanes + 无每步 carry check 分支）× IPC ↑2.4x ≈ 25x 理论上限，实测 21x（差异来自 SIMD 端 8 宽但 32 位加法流水线宽度、非理想向量化）。

---

## 四、🎯 最纯粹的 Core_Bound 教训：**消依赖 > 摆布局 > 一切微操**

对照 47 号 vectorization_1：

| | **47 vectorization_1**（只摆 SoA） | **48 vectorization_2**（消依赖链） |
|--|---------------------------------|--------------------------------|
| 做了什么 | AoS→SoA 转置让数据连续 | 拓宽累加器消除循环依赖 |
| 依赖链 | **仍在**（DP 天然有 row→row 依赖）| **消除了**（循环内只剩可结合的 +）|
| 编译器能 SIMD 吗 | 部分（内层 k 循环可 SIMD，外层依赖卡死）| **完全可以**（width 8 全线并行）|
| 加速 | **2.1x** | **21.0x** 🔥 |

> **10 倍差距**——同样在 Core_Bound 药箱里，"消依赖"这一招**比"摆布局"高一个数量级**。第 6 课"`for k` 里的 `sum += a[k]` 归约要 `-ffast-math`"、第 5 课"标量重命名/扩展"、以及本实验的"拓宽累加器 + 末尾折回"，**都是同一件事**：让循环内的操作**变得可交换/可结合**，编译器就敢并行。

---

## 五、`-Rpass` 交叉验证的教科书示范

这个实验把 4 号文档反复讲的"**让编译器交代**"用到了极致：
- **基线**：`-Rpass-missed=loop-vectorize` → `loop not vectorized` ⚠️
- **优化**：`-Rpass=loop-vectorize` → `vectorized loop (width 8)` ✅

**两行报告**直接把 21x 加速的**根因锁死在"是否向量化"上**——不用猜、不用查汇编。这就是**性能工程师的"证据链"**：代码变化 → 编译器报告变化 → 数字变化，三者闭环。

---

## 六、更普适的"消依赖链"三板斧（工程复用）

本实验的手法可归纳成三类，都能套用到真实代码：

| 招式 | 本实验的做法 | 通用例子 |
|------|------------|---------|
| **拓宽类型** | uint16 → uint32 | int 累加换 int64；定长缓冲留足余量避免循环内溢出检查 |
| **推迟修正** | 循环内 add-carry → 循环外一次性 fold | 归约里减少 max/min/normalize；把 clamp/normalize 移出热循环 |
| **多累加器** | (可进一步) 开 2-4 个 uint32 交错累加打破加法延迟链 | sum += a[i]; sum2 += a[i+1]; ... 末尾合并 |

第三条 `vectorization_2` 本实验没做（SIMD 已经做了），但**在手写 SIMD 或编译器不自动向量化时威力极大**（对应 `dep_chains_1` 用 M 路并行破链表）。

---

## 七、放进 `20 §一` 闭环

| 步骤 | 本实验 |
|------|--------|
| ① 度量基线 | bench 24.8 μs |
| ② 定位热点 | `checksum` 循环里的 add + carry 检查 |
| ③ TMA 分诊 | Core_Bound 74.9%（IPC 卡在 2.46）+ `-Rpass` 报"未向量化" |
| ④ 开药 | **拓宽累加器 + 末尾折回**（消依赖链）|
| ⑤ 验证 | 21.0x + `-Rpass` 报"已向量化 width 8" + IPC 5.80 = **命中根因** ✅ |

---

## 八、一句话收口

> **依赖链是 SIMD 的头号杀手**——`acc += acc < value` 这一行看似无害，让 64K 次迭代彻底串行。**拓宽累加器 + 末尾一次性折回**就把循环内变成"可结合的纯加法"→ 编译器立刻 8-lane AVX2 → **21x 加速**。同 47 号"只摆 SoA 不消依赖 = 2.1x"合读，就能理解第 5/6 课反复讲的"**消依赖 = 最高杠杆**"到底是多大的杠杆。

## 九、延伸

- 相关理论：`06-单核ILP-SIMD.md § 三`（SIMD + 归约）、`05-程序编写优化.md § 六`（去除相关性三兄弟）、`04-编译器概述.md § 一 4`（`-Rpass`）
- 直接对照：`47-实战-vectorization_1.md`（未消依赖，2.1x）
- 相关实验：`dep_chains_1`（第 51 号，M 路并行破链表，同类"消依赖"）
- 原始工程文档：[`docs/vectorization_2.md`](../experiments/core_bound/vectorization_2.md)
