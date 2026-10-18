# 47 · 实战 · `vectorization_1`（Smith-Waterman 序列比对 AoS→SoA）

> **TMA 一级**：**Core_Bound** · **优化技术**：**AoS→SoA 转置 + 让编译器 SIMD 内部 k 循环** · **工程加速比 2.1x**
>
> ⚠️ **本实验**：源码内 scalar/SIMD 两版通过注释块共存，本机复现时脚本切换未一次跑通（编译报 redefinition）。**本文记录代码结构 + 机制分析 + 引用工程实测数据**，供复现者参考改进自动化脚本。

---

## 一、实验简介

**任务**：Smith-Waterman 局部序列比对。有 **16 条序列（每 200 碱基）**，需**分别**算两两比对的最高分。

**数据布局关键**（本实验的靶心）：

```
AoS (原始):  vector<array<uint8_t, 200>>   // 16 条序列, 每条连续 200B
             ├ seq0: [b,b,b,b,...]         // 逐条 DP, 每步 int16 标量
             ├ seq1: [b,b,b,b,...]
             └ ...

SoA (优化):  array<array<int16_t, 16>, 200> // transpose 后: 每"位置"含 16 条序列的该位
             ├ pos0: [s0[0], s1[0], ..., s15[0]]   ← 16 个字节, AVX 一次装完
             ├ pos1: [s0[1], s1[1], ..., s15[1]]
             └ ...
```

**转换函数**（solution.cpp 顶部已提供 `transpose()`）：
```cpp
simd_sequence_t transpose(std::vector<sequence_t> const &sequences) {
    simd_sequence_t simd_sequence{};
    for (size_t i = 0; i < simd_sequence.size(); ++i)
        for (size_t j = 0; j < sequences.size(); ++j)
            simd_sequence[i][j] = sequences[j][i];   // 关键转置
    return simd_sequence;
}
```

---

## 二、两版 `compute_alignment` 对比

### 基线（当前 solution.cpp 里的）：**逐序列 scalar DP**
```cpp
for (size_t sequence_idx = 0; sequence_idx < 16; ++sequence_idx) {
    score_t gap_open{-11}; ...          // 每序列各初始化一次
    for (col = 1; col <= 200; ++col)
        for (row = 1; row <= 200; ++row) {
            score_t best = last_diagonal + (seq1[row-1]==seq2[col-1] ? match : mismatch);
            best = max(best, last_vertical);
            ...
        }
}
```
- **`sequence_idx` 循环**：16 条序列**独立**跑完整 DP
- **DP 内层用标量** `int16_t`（一次算 1 个格子）

### 优化（注释块里的）：**16 序列 SIMT 并行**
```cpp
auto trSeq1 = transpose(sequences1);   // 转置: 每"格子"变成 16 元 int16 向量
auto trSeq2 = transpose(sequences2);
using score_t = simd_score_t;          // array<int16_t, 16>  ← 关键换类型
...
for (col = 1; col <= 200; ++col)
  for (row = 1; row <= 200; ++row) {
    for (size_t k = 0; k < 16; ++k) {              // ← 编译器 SIMD 掉这个 k 循环
      best_cell_score[k] += (trSeq1[row-1][k]==trSeq2[col-1][k] ? match[k] : mismatch[k]);
    }
    for (size_t k = 0; k < 16; ++k) {
      best_cell_score[k] = std::max(best_cell_score[k], last_vertical_gap[k]);
      ...
    }
  }
```

**核心机制**：
- **数据从 AoS 摆成 SoA**（`transpose`）→ 16 条序列同一位置的字节**连续存放**
- **DP 主循环结构基本不变**，但把 `score_t` 从 `int16_t` 换成 `array<int16_t, 16>`
- **`for k in 0..16` 循环**：16 元素紧密、无分支 → **编译器自动 SIMD** 成 1-2 条 AVX2 指令（256 位 = 16 个 int16）
- **一次 DP 同时算 16 条序列** → 逻辑并行度 ×16，但被 400 vs 400 μs 数字（2.1x）稀释说明开销也在（转置代价 + 更多 max/分支）

---

## 三、为什么加速只有 2.1x？

理论上"16 序列并行"应该 8-16x，实际只有 2.1x。**Core_Bound 优化典型的"折损"**：

| 因素 | 折损原因 |
|------|---------|
| **转置代价 O(16 × 200)** | 一次性开销，但相比 DP 的 200 × 200 × 16 = 640k 步，只占 0.5% |
| **每 cell 有大量 `max` 与条件** | SIMD 里 max 是好的、但 `?:` 三元要靠 blend 指令，成本高于标量分支 |
| **DP 有真依赖** | `last_diagonal_score = score_column[row]` → 前后 row 有真依赖（第 6 课讲过），SIMD 帮不上 |
| **AVX2 int16 只有 16 lane** | 一次算 16 条 = 一次填满一个 ymm；理论上限 16x，但依赖链 + 分支吃掉了大部分 |

**2.1x 是"依赖有限的 DP + SIMD 部分并行"的合理结果**。

---

## 四、`-Rpass` 验证 SIMD 是否真的生效（推荐排查手段）

复现者验证本机 SIMD 有没有被激活：
```bash
clang++ -O3 -march=native -Rpass=loop-vectorize \
        -Rpass-missed=loop-vectorize solution.cpp -c
# 期望: 看到 "vectorized loop (vectorization width: 16, interleaved count: 1)" 或 width 8
```

若 width = 8（一次 8 个 int16 = 128 位 SSE）而非 16 → 需 `-mavx2` 才能拿 256 位。
若 `loop not vectorized: dependence` → 说明有依赖没消（对照 `48 vectorization_2` 的消依赖技巧）。

---

## 五、放 Core_Bound 药箱里看

| 手法 | 本实验用了什么 |
|------|--------------|
| **AoS→SoA**（第 6/7 课）| ✓ transpose() 转置为 SIMD 友好布局 |
| **消依赖**（第 6 课）| ✗ DP 依赖链仍在 → 收益打折 |
| **FMA / SIMD 手动**（第 6 课）| ✗ 只依赖编译器自动向量化 |
| **循环展开**（第 6 课）| 部分（16 元 array 隐式展开）|

**结论**：**光做 AoS→SoA 而不消依赖 → 只吃 2.1x**（对比 48 号"消依赖链"能拿 20.9x，差距一目了然）。这正好为 48 号铺垫。

---

## 六、放进 `20 §一` 闭环

| 步骤 | 本实验 |
|------|--------|
| ① 度量基线 | 本机 build/lab = **403 μs** |
| ② 定位热点 | `compute_alignment` 三重循环 DP |
| ③ TMA 分诊 | 期望 IPC 低但 cache-miss 不高 → **Core_Bound** |
| ④ 开药 | **AoS→SoA 转置 + SIMD-friendly 数据结构** |
| ⑤ 验证 | 工程 2.1x；本机因脚本切换未跑通 build，但**代码结构差异清晰可解释** |

> 📝 诚实备注：本实验**未在本机跑出 SIMD 版数字**（Python 换源脚本处理注释块的边界不稳）。改进方向：把 scalar 版和 SIMD 版**分文件**存（如 `solution_scalar.cpp` / `solution_simd.cpp`），build 时用 CMake 选一份，避免运行时注释切换。

---

## 七、一句话收口

> **AoS→SoA 是 SIMD 的必要非充分条件**——本实验转置了、结构改好了，但**依赖链没消**，只赚 2.1x。这为 48 号 `vectorization_2`（**消依赖链 → 20.9x**）埋下最漂亮的对照：**同是 Core_Bound，"消依赖"是比"摆布局"高一个数量级的杠杆**。

## 八、延伸

- 相关理论：`06-单核ILP-SIMD.md § 三`（SIMD + 消依赖）、`07-访存优化.md § 六`（AoS/SoA）
- 直接对照下一实验：**48-实战-vectorization_2.md**（消依赖链，20.9x）
- 原始工程文档：[`docs/vectorization_1.md`](../experiments/core_bound/vectorization_1.md)
