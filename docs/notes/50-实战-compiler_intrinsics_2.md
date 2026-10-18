# 50 · 实战 · `compiler_intrinsics_2`（AVX2 手写批量查找换行符）

> **TMA 一级**：Core_Bound + Bad_Speculation 混合 · **优化技术**：**AVX2 手写 32 字节一次找 `\n`** · **加速比**：**13.7x 本机** ≈ 15.1x 文档
>
> ⭐ **49 号的姊妹对照**：同为"手写 SIMD"，49 号只赚 1.14x、50 号赚 13.7x——**"手写值不值"取决于编译器基线的天花板**。

---

## 一、读题（Step 0）

**任务**：给一个字符串（本实验输入 `inputs/` 里几份 .txt），找出**最长行的字符数**。逐字节扫描 + 计数 + 遇 `\n` 更新最大值。

**基线**：
```cpp
uint32_t solution(const std::string &inputContents) {
    uint32_t longestLine = 0, curLineLength = 0;
    for (char c : inputContents) {
        if (c == '\n') {                                    // ← 数据相关分支
            if (curLineLength > longestLine) longestLine = curLineLength;
            curLineLength = 0;
        } else {
            curLineLength++;
        }
    }
    ...
}
```

**优化（solution.cpp 里）**：AVX2 `_mm256_cmpeq_epi8(v, eol)` 一次比较 32 字节 → 用 bitmask + `__builtin_ctz` 定位 `\n` 位置，跳过无换行的 32 字节块。

---

## 二、Step ①②③④ 本机实测

```bash
cd TMA/core_bound/compiler_intrinsics_2
# 用 python 写基线 (避免 shell 对 '\n' 的转义陷阱, 用整数 10 代替)
cmake --build build_base --parallel 4
INP=$(pwd)/inputs
taskset -c 0 ./build_base/lab "$INP"
taskset -c 0 ./build/lab "$INP"
```

| | 基线 scalar | 优化 AVX2 手写 | 变化 |
|--|-----------|--------------|------|
| **Google Bench** | **151 μs** | **11.0 μs** | **13.7x** ✅ |
| cycles (cpu_core 总) | 4.68 B | 4.40 B | ≈ |
| instructions (总) | 19.01 B | 23.41 B | 略多 |
| **IPC** | 4.06 | 5.32 | ↑ 1.3x |
| **branch-misses** | **57.2 M** 🤯 | **5.44 M** | **↓ 10.5x** |
| cache-miss | 138 K | 131 K | ≈ |

**每 iter 归一**：
- 基线 4620 iters → **12,400 次分支失败 / iter**
- 优化 60011 iters → **~91 次 / iter**
- 差 **136 倍**

---

## 三、🎯 为什么 50 号能赚 13.7x，49 号只赚 1.14x？

同为"手写 SIMD"，收益差 **12 倍**。关键差异：

### 49 号（前缀和 imageSmoothing）
- 基线 scalar 版本**能被编译器自动 SIMD**（IPC 3.93，只差一点点没做到极致）
- 手写 SSE4.1 需要"3 shift + 3 add"做 8 元前缀和 → 收益被 SIMD 内部开销吃光
- **净赚 14%**

### 50 号（找 `\n`）
- 基线**每字符一个数据相关分支**（`if (c == '\n')`）→ **分支预测器疯狂失败**（每 iter 12400 次误判）
- 编译器**很难自动 SIMD 掉带状态 (`curLineLength`) 的分支循环**
- 手写 AVX2 一次比较 32 字节 → **完全绕开逐字符分支**，把分支密度砍 100x
- **净赚 13.7x**

> 💡 **一句话原则**：**手写 SIMD 的收益 ≈ "编译器基线天花板 − 手写 SIMD 能做到"**。基线已经 IPC 3.9 → 天花板高，手写赚不了多少；基线被 branch-miss 卡爆 → 天花板低，手写才有空间。

---

## 四、Branch-Miss 才是本实验真正的敌人

看 IPC 只 4.06（看似很高，接近大核峰值 4-6）——但**每 iter 时间 151μs** 依然长。为什么？

```
151 μs × 3 GHz ≈ 450,000 cycles / iter
12,400 branch-misses × 每次误判 ~15-30 cycles 惩罚 ≈ 180K-370K cycles 花在流水线清空
→ 相当于每 iter 有一半到 80% 时间在做 "predict → flush → re-fetch"
```

**IPC 4.06 是"成功执行的指令"密度**——**不包括被误判作废的**！这就是为什么 49 号实验反复强调 "**IPC 单独看会骗人**"——本实验 IPC 好看但真瓶颈是 Bad Speculation + Core_Bound 混合。

> 这完美演示了 **`20 §一` 闭环的"TMA 分诊"步骤为什么必须看 branch-miss**：如果只看 IPC，你会以为基线"已经很快了，没优化空间"。

---

## 五、Core_Bound 手法优先级表（48+49 合璧验证）

| 优先级 | 手法 | 何时用 | 本系列实证 |
|-------|------|-------|-----------|
| 🥇 **消依赖链** | 让循环可交换/结合 | **永远先想** | **48 号 21x** |
| 🥈 **摆布局**（AoS→SoA） | 数据连续利于 SIMD | 依赖已消但数据散 | 47 号 2.1x |
| 🥉 **编译器辅助** | `restrict`、`-ffast-math` | 让 SIMD 敢开 | 47 靠"拓宽" |
| **4️⃣ 手写 intrinsics** | 编译器天花板低时才用 | 数据相关分支、gather、位操作 | **50 号 13.7x ✅ / 49 号 1.14x ❌** |

**48 vs 49 是"何时该手写"的教科书对照**：
- **先看基线 branch-miss 和 IPC**——若 IPC 已 3+ 且 branch-miss 很少（49 号），**放弃手写**；
- **若 branch-miss 爆表**（本 50 号 12K/iter）或有 SIMD 无法表达的位操作 → **手写值得**。

---

## 六、复现踩到的坑（3 个）

1. **heredoc 里的 `\n`**：bash 双引号里 `\n` 可能被转义，导致 C 源码里的 `if (c == '\n')` 变成 `if (c == '\\\n')` 编译错。
   **解法**：用**整数 `10`**（ASCII 换行）代替字面量。
2. **`uint32_t` 缺 include**：基线独立文件不继承原文件头。
   **解法**：加 `#include <cstdint>`。
3. **命令行参数**：本实验 `lab` 需要传 `inputs/` 目录（`Usage: lab path/to/inputs [gbench args]`）。

---

## 七、放进 `20 §一` 闭环

| 步骤 | 本实验 |
|------|--------|
| ① 度量基线 | 151 μs |
| ③ TMA 分诊 | **IPC 4.06 骗人** → 深挖 branch-miss 12K/iter → **Bad_Spec + Core_Bound 混合** |
| ④ 开药 | **AVX2 一次 32 字节比较**绕开逐字符分支 |
| ⑤ 验证 | 13.7x + branch-miss ↓10.5x ✅ |

---

## 八、一句话收口

> **手写 SIMD 什么时候值得？——基线 branch-miss 爆表、IPC 又高（假象好看）的时候**。50 号用 12400 次分支失败/iter 与 13.7x 加速，与 49 号 IPC 3.93 已近峰值、手写只赚 1.14x 形成**最锋利的对照**——同样是"手写 SIMD"，选对场景是 12 倍差距。**分诊决定收益**，不是代码写得越复杂就越快。

## 九、延伸

- 相关：`04-编译器概述.md § 三.4`（ymm 与手写宽度锁定）、`06-单核ILP-SIMD.md § 三.2.4`（分支向量化）
- 姊妹实验：**49 compiler_intrinsics_1**（1.14x，收益小的反例）
- 原始工程文档：[`docs/compiler_intrinsics_2.md`](../experiments/core_bound/compiler_intrinsics_2.md)
