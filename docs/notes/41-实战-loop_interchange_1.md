# 41 · 实战 · `loop_interchange_1` 完整闭环

> **立意**：第 5 阶·perf-ninja 第 2 个实验（warmup 之后）。这是把 `20` 号"诊断→开药→验证"闭环**在工业级 benchmark 框架上跑通**的完整记录。Google Benchmark + CMake + 绑核 + perf 硬计数器 —— **每一步都能自己复现**。

**实验目录**：[`TMA/memory_bound/loop_interchange_1/`](../../TMA/memory_bound/loop_interchange_1/)
**原始文档**：[`docs/loop_interchange_1.md`](../experiments/memory_bound/loop_interchange_1.md)（工程侧记录，本文档是**本机实测版**）
**目标加速比**：6.9x（工程历史值）· **本机实测**：**6.81x** ✅

---

## 一、读题（Step 0）

### 结构
```
loop_interchange_1/
├── CMakeLists.txt        # 引用 tools/labs.cmake, 生成 lab/validate 两个 target
├── bench.cpp             # Google Benchmark 入口: bench1 跑 power(a, 2021) 10 次
├── init.cpp              # 用 <random> 生成 400×400 float 矩阵, 每行归一化
├── validate.cpp          # 正确性校验
├── solution.h            # N=400, float 方阵; 声明 multiply/power
├── solution.cpp          # 当前为"优化后"版 (i-k-j)
├── solution.cpp.ijk.bak  # 慢版基线 (i-j-k)
├── solution.cpp.ikj.bak  # 快版备份 (i-k-j)
├── build_ijk/lab         # 已用 ijk 源码编译的基线二进制 ← 我们直接跑它
└── build/lab             # 已用 ikj 源码编译的优化二进制
```

### 核心代码：矩阵幂 `power(a, 2021)` = ~11 次矩阵乘（二进制快速幂）

**慢版 i-j-k**（内层遍历 k，`b[k][j]` 跨行**跳步**访问）：
```c
for (i) for (j) { result[i][j] = 0;
  for (k) result[i][j] += a[i][k] * b[k][j];   // ← b 步长 400, 不能 SIMD
}
```

**快版 i-k-j**（内层遍历 j，`result[i][j]` 与 `b[k][j]` **都连续**）：
```c
for (i) for (k) { float t = a[i][k];
  for (j) result[i][j] += t * b[k][j];         // ← 两个都连续, 8 宽 SIMD
}
```

### 编译配置（`labs.cmake` 默认）
`-O3 -march=native -ffast-math`（三件套都开），C++17。

---

## 二、Step ① 基线 bench（Google Benchmark）

```bash
cd TMA/memory_bound/loop_interchange_1
taskset -c 0 ./build_ijk/lab
```

**输出**（`Iterations(10)` 是 bench.cpp 硬编码）：
```
--------------------------------------------------------------------------------
Benchmark                     Time             CPU   Iterations
--------------------------------------------------------------------------------
bench1/iterations:10  405083327 ns    405075119 ns           10
```

**单次 `power(a,2021)` ≈ 405 ms**；系统 `time` 三值：real 4.057s / user 4.048s / sys 0.009s → CPU 密集（`real ≈ user + sys`），符合第 1 课判断。

> ⚠️ Google Benchmark 打了 `***WARNING*** CPU scaling is enabled`——**说明这台机器没锁频率**（`30` 手册纪律 2）。虽然绑核 + 密集计算让噪声可控，但严格实验前应 `sudo cpupower frequency-set --governor performance`。

---

## 三、Step ② 基线 perf 硬件计数器（分诊）

```bash
perf stat -e cycles,instructions,cache-references,cache-misses,branch-misses,branch-instructions \
  taskset -c 0 ./build_ijk/lab
```

本机是 **Intel 混合架构**（P 核 `cpu_core` + E 核 `cpu_atom`）。**99.99% 时间在 `cpu_core`**，读大核即可（对应 `30` 陷阱 6）：

| 指标（cpu_core） | 基线 |
|-----------------|------|
| cycles | 21.95 B |
| instructions | 92.76 B |
| **IPC = inst/cyc** | **4.23** |
| cache-references (LLC) | 2.99 M |
| cache-misses (LLC) | 1.13 M |
| LLC miss 率 | **38%** |
| branch-misses | 28.99 M / 11.6 B 分支 = **0.25%** |

---

## 四、Step ③④ 上药 + 复测（对比表）

```bash
taskset -c 0 ./build/lab
perf stat -e ... taskset -c 0 ./build/lab
```

**完整前后对照**（`cpu_core` 视角）：

| 指标 | 基线 i-j-k | 优化 i-k-j | 变化 |
|------|-----------|-----------|------|
| Google Bench 单次 | 405 ms | **59.5 ms** | **加速 6.81x** 🔥 |
| 系统 elapsed | 4.07 s | 0.61 s | 6.7x |
| **instructions** | 92.76 B | **8.94 B** | **↓ 10.4x** ⭐ |
| cycles | 21.95 B | 3.27 B | ↓ 6.7x |
| IPC（原始）| 4.23 | 2.73 | ↓ ⚠️ 见 §六 |
| cache-misses（LLC） | 1.13 M | 465 K | ↓ 2.4x |
| **LLC miss 率** | 38% | **21%** | ↓ 17pp |
| **branch-miss 率** | 0.25% | **0.033%** | ↓ **7.5x** |

---

## 五、加速比 & 与工程文档对比

**本机 6.81x ≈ 工程文档 6.9x**（RTX 时代不同机器）。差 0.09 属于机器/编译器噪声。**这是本套笔记里第一个"跨机器加速比高度吻合"的实验**——反证前面 ⑤/⑥/⑨b/⑩b 的"负结果"不是玄学，而是**硬件/负载真的变了**（对应 `30` 陷阱 13）。

---

## 六、🎯 深度解读：这次不是"单纯 cache miss"，而是"SIMD 被激活"

**最反直觉的观察**：`IPC 从 4.23 掉到 2.73`——**看起来 CPU 每周期做的更少**！这桩"惨案"其实是好事，让我把三件事拆开：

1. **`instructions` 少了 10.4 倍**（92.76B → 8.94B）——**这才是核心信号**
   - 慢版 i-j-k 内层沿 k 循环，`b[k][j]` 跨步 → **编译器无法向量化**（`-Rpass-missed` 会显示 stride 问题）
   - 快版 i-k-j 内层沿 j 循环，`b[k][j]` 与 `result[i][j]` 都连续 → **AVX2 一次 8 个 float**
   - 每条向量指令 = 逻辑上 8 条标量指令 → 指令数直接砍到 1/8~1/10
   - **这是第 4 课 `-Rpass` + 第 6 课 SIMD + 第 5 课循环交换三课的**同时**兑现

2. **真实"逻辑吞吐"暴涨**（这才是 IPC 该看的口径）
   ```
   基线:  4.23 scalar-ops / cycle
   优化:  8.94B × ~8 lanes / 3.27B cyc ≈ 21.9 logical-ops / cycle
   ```
   **实际每周期做了 5.2 倍的有效工作**。原始 IPC 下降是**假象**——SIMD 指令每条要 4+ 周期退休，但每条干 8 倍活。
   > 👉 呼应 `30` 手册元教训 2：**单一视角会骗人**。IPC 单独看会误判"变慢了"；必须配合 `instructions` 绝对数 + 是否 SIMD。

3. **`cache-miss` 也降但**幅度小**（LLC 层 38%→21%）**
   - N=400 float 三矩阵 ≈ 1.9 MB，**大部分能塞进 L3**（本机 L3 数十 MB），所以 LLC 层不是主瓶颈
   - 真正的收益来自 L1/L2 层的**连续访问 + 更少访存次数**（SIMD 一条 load 拉 8 个）
   - **branch-miss 降 7.5x**：SIMD 让 1 条分支覆盖 8 个元素 → 分支总数稀释

---

## 七、放进 `20` 复盘的闭环对照

| 闭环步骤 | 本实验具体动作 | 关键数字 |
|---------|--------------|---------|
| ① 度量基线 | Google Bench 单次 power | 405 ms |
| ② 定位热点 | 已知就是 `multiply` 三重循环 | — |
| ③ TMA 分诊 | perf 计数：IPC 4.23 + cache miss 高 + **指令数 92B 巨大** | 后向量化能救 |
| ④ 开药 | **循环交换 i-j-k → i-k-j**（一次改法同时解锁 SIMD + 局部性）| — |
| ⑤ 验证 | 加速 6.81x + 指令数 10.4x 减 + SIMD 生效 | **命中预期** |

---

## 八、复现命令清单（本机可跑）

```bash
# 一次性完整复现 (前提是 libbenchmark.a 已在 tools/benchmark/build/src/, build_ijk/ 已编译)
cd TMA/memory_bound/loop_interchange_1

# 基线 + 优化 各自 bench
taskset -c 0 ./build_ijk/lab     # 405 ms
taskset -c 0 ./build/lab         # 59.5 ms

# 各自 perf 硬件计数器
perf stat -e cycles,instructions,cache-references,cache-misses,branch-misses,branch-instructions \
  taskset -c 0 ./build_ijk/lab
perf stat -e cycles,instructions,cache-references,cache-misses,branch-misses,branch-instructions \
  taskset -c 0 ./build/lab

# 若从零重建 (源改回慢版):
cp solution.cpp solution.cpp.ikj.bak
cp solution.cpp.ijk.bak solution.cpp
cmake -E make_directory build_x && cd build_x
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_FLAGS="-g" -DCMAKE_CXX_FLAGS="-g" ..
cmake --build . --parallel 8
./validate && ./lab
```

---

## 九、收口：这个实验教会我们的三件事

1. **"改一次循环顺序"同时打开两重收益**——SIMD + 局部性；加速 6.8x **不是单一因素**，是第 5 课"消依赖暴露并行"（SIMD 那条）+ "提升局部性"（缓存那条）**两条一起兑现**。
2. **IPC 不是越高越好的绝对真理**——SIMD 会让 IPC 数字下降但吞吐上升。**看性能必须看指令绝对数 + 类型**（scalar / vector）。这是 `30` 元教训 2 的最新案例。
3. **工程文档的 6.9x 我们几乎完美复现（6.81x）**——说明 perf-ninja 这类"标准实验"的价值：**跨机器仍有可比性**，只要负载是计算密集 + 编译器版本相近。反面对照：**只有"跨代差异大 / 单机 vs 集群"这种前提变化了才会失效**（如 ⑨b/⑩b）。

## 十、延伸

- 复盘工作流：`20-单核优化全景复盘.md`
- 度量纪律：`30-测量纪律手册.md`（尤其是"IPC 单独看会骗人"）
- 相关实验代码：`TMA/memory_bound/loop_interchange_2/`（高斯模糊，9.7x）
- 相关工具实战：`docs/intel_advisor.md`（同一实验的 Roofline 分析版）
