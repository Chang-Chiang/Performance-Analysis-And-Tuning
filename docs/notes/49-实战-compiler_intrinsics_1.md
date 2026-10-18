# 49 · 实战 · `compiler_intrinsics_1`（SSE4.1 手写图像平滑 vs 编译器自动向量化）

> **TMA 一级**：Core_Bound · **优化技术**：**SSE4.1 手写 intrinsics 前缀和** · **加速比**：**1.14x 本机** ≈ 1.14x 文档
>
> 💡 **本实验的独特价值 = "低收益本身就是最重要的教训"**：手写 intrinsics 只赢编译器 14%，正是 04 §三"信任编译器 > 手写"的量化标本。

---

## 一、读题（Step 0）

**任务**：`imageSmoothing`——滑动窗口求和（每个输出 = 附近 `2*radius+1` 个像素的和），本质是**前缀和**的差分形式。基线是标量循环，本实验**手写 SSE4.1** 试图加速。

**优化版核心代码骨架**（solution.cpp 里 `for (; i + 7 < limit - pos; i += 8)` 主循环）：
```cpp
__m128i sub = _mm_cvtepu8_epi16(_mm_loadu_si64(subtractPtr + i));   // 出窗元素
__m128i add = _mm_cvtepu8_epi16(_mm_loadu_si64(addPtr + i));        // 入窗元素
__m128i diff = _mm_sub_epi16(add, sub);                              // 每步的差

// 关键: 用 3 次 shift + 3 次 add 做 8 元素前缀和 (log2 8 = 3 步)
__m128i s = _mm_add_epi16(diff, _mm_slli_si128(diff, 2));
s = _mm_add_epi16(s, _mm_slli_si128(s, 4));
s = _mm_add_epi16(s, _mm_slli_si128(s, 8));

__m128i result = _mm_add_epi16(s, current);
_mm_storeu_si128((__m128i*)(outputPtr + i), result);
currentSum = (uint16_t)_mm_extract_epi16(result, 7);                 // 提取末尾作 scalar
```

**基线** = 禁用上面主循环，只跑剩下的"Remainder: sequential loop"（纯标量，让编译器决定要不要 SIMD）。

---

## 二、本机实测

```bash
cd TMA/core_bound/compiler_intrinsics_1
cp solution.cpp solution.cpp.opt.bak
python3 -c "s=open('solution.cpp').read(); s=s.replace('for (; i + 7 < limit - pos; i += 8) {', 'for (; false; i += 8) {'); open('solution.cpp','w').write(s)"
# cmake build_base ...
```

**基线**（纯标量，让 `-O3` 自动向量化）：`bench_partial_sum  15.1 us`
**优化**（手写 SSE4.1）：`bench_partial_sum  13.2 us`

**加速比 = 15.1 / 13.2 = 1.14x** ✅ 完美匹配文档。

**perf**（cpu_core 视角）：

| 指标 | 基线 | 优化 | 反差 |
|------|-----|-----|------|
| 每 iter 指令 | 402 K | **105 K** | 手写少 **3.83x** ✓ |
| 每 iter cycles | 102 K | 87 K | 少 1.17x |
| **IPC** | **3.93** | **1.21** | 反而降 |
| cache-miss | 86 K | 76 K | 差不多 |
| branch-miss | 121 K | 128 K | 差不多 |

---

## 三、🎯 为什么只赢 14%？——三重"抵消"

### 抵消 1：**编译器基线已经做得很好**
- 基线 IPC = **3.93**（很高！接近大核峰值 4-6）→ `-O3 -march=native` **已经自动向量化**了 remainder loop
- 也就是说：你以为在跟"标量循环"比，其实在跟"编译器帮你写好的 SIMD"比

### 抵消 2：**前缀和的 SIMD 天然不便宜**
- 每 8 元素前缀和需要 **3 次 shift + 3 次 add**（log2 8 步的 Hillis-Steele）
- 而不是 1 次加法 → SIMD 优势从"8 倍"缩水到"8/6 ≈ 1.3x"
- 加上首末 extract、broadcast、load/store 开销 → 净赚只剩 14%

### 抵消 3：**优化版 IPC 只有 1.21**
- 手写 SIMD 每条指令做 8 份工作，退休速度反而慢（宽指令更吃流水线）
- 呼应 **30 元教训 2**："**IPC 单独看会骗人**"——但**这次**是反向：基线 IPC 高、优化 IPC 低，**但优化仍然更快**（因为每条 SIMD 干的活多）

---

## 四、和 4 号文档 "手写 intrinsics 锁死宽度" 呼应

- **04 §三.4 实验 ④**：硬编码 `__m128` → AVX2 下 `ymm=0`，编译器**不会**把它撑宽到 256 位
- **本实验**：手写了 SSE4.1（`_mm_cvtepu8_epi16` 等），**宽度锁死在 128 位 / 8 元素**
  - 而编译器 `-O3 -march=native` 有可能把 scalar 版本向量化成 **AVX2 256 位 / 16 元素**
- 换句话说：**手写 128 位反而可能比让编译器自动 256 位更亏**。本实验里没发生（因为前缀和 SIMD 结构复杂，编译器可能也只做到 128），但风险存在。

---

## 五、把"手写 intrinsics"放回它该在的位置

**Core_Bound 手法优先级**（49 号实验用血泪数字佐证）：

| 优先级 | 手法 | 何时用 |
|-------|------|-------|
| 🥇 **消依赖链**（48 号） | 让循环内操作可交换/结合 → 编译器自动 SIMD | **永远先想这一步**，收益能到 10-20x |
| 🥈 **摆布局**（47 号） | AoS→SoA、结构体紧凑 | 依赖已消但数据不连续时 |
| 🥉 **编译器辅助** | `restrict`、`-ffast-math`、`-march=native` | 解锁编译器 SIMD 决策 |
| **4️⃣ 手写 intrinsics** | 只在**上面三招都不管用**时用 | 前缀和、gather、位操作等 SIMD 特殊模式 |

**49 号正好在 4️⃣ 那格**——它证明了"前缀和确实需要手写 SIMD"（编译器无法把有依赖的累加自动向量化到 SIMD 前缀和），但也证明了**这种手写的收益是"个位数百分点"级别**（1.14x）——**投入产出比最差的一类优化**。

---

## 六、放进 `20 §一` 闭环

| 步骤 | 本实验 |
|------|--------|
| ① 度量基线 | 15.1 μs（scalar remainder loop，编译器已 SIMD 化到 IPC 3.93）|
| ③ TMA 分诊 | **IPC 3.93** = 基线**已经不是 Core_Bound 大头**——没有"20x 空间"给你抢 |
| ④ 开药 | 手写 SSE4.1 前缀和（3 shift + 3 add 做 log2(8) 归约）|
| ⑤ 验证 | **1.14x** — 收益小，但**这就是真相**：不是所有优化都值得做 |

---

## 七、一句话收口

> **"手写 SIMD 只赢编译器 14%"** —— 这是整套 16 实验里 **投入产出比最低** 的一个，但**教育意义极高**：
> - **别默认"手写 SIMD 一定更快"**——`-O3 -march=native` 已经把 IPC 拉到 3.93，你很难打过
> - **别默认"能优化就一定值得"**——1.14x 收益 = 复杂代码 + 移植性损失 + 维护成本，工程上经常**不做更好**
> - **优先级永远是"消依赖 > 摆布局 > 编译器解锁 > 手写 intrinsics"**（对照 48 号 21x 的悬殊差距）

## 八、延伸

- 相关：`04-编译器概述.md § 三.4`（手写 intrinsics 锁死宽度实验）、`06-单核ILP-SIMD.md § 三`
- 对照：**48 号 21x** vs 本实验 1.14x → "选对手法"比"努力程度"重要得多
- 姊妹实验：**50 compiler_intrinsics_2**（AVX2 手写换行符查找，**15.1x**）——同样是手写 SIMD，为什么这次赚这么多？下篇揭晓。
