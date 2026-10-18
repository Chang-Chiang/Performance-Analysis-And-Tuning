# 06 · 单核优化：ILP + SIMD

> **立意**：治 `Core_Bound` 的两味主药——**ILP** 让 CPU **同一周期炒不同指令**（多指令并行）；**SIMD** 造**一口大锅一次炒 4/8 份**（一指令多数据）。所有 `core_bound` 系实验（vectorization/compiler_intrinsics/dep_chains）的理论根基。

---

## 一、两种并行对照（务必分清）

| | **ILP 指令级并行** | **SIMD 数据级并行** |
|---|---|---|
| 并行的是 | 不同指令 | 同一指令的多个数据 |
| 硬件靠 | 流水线 + 超标量 + 乱序 | 向量单元 (SSE/AVX) |
| 你要做的 | 打破指令依赖、循环展开 | 把数据摆成向量、消依赖 |
| 收益指标 | 提升 **IPC**（填满发射槽）| 一条指令算 4/8 个 |

---

## 二、ILP：流水线 + 4 种相关性 + 多发射

### 1. 流水线（5 段）
```
取指 → 译码 → 执行 → 访存 → 写回
指令1:  █
指令2:     █
指令3:        █
```
**IPC**（第 2 课）= 流水线填满程度。**超标量** x86/ARM（硬件动态调度）；**VLIW** DSP（编译器静态排 `‖`）。

### 2. 4 种相关性 → 会卡住流水线

| 相关性 | 情形 | 消除手法 |
|--------|------|---------|
| **数据相关(真依赖)** | B 读 A 刚写的 | 只能重排别的指令 / 展开 |
| **反相关 (anti)** | A 先读 R，B 后写 R | **寄存器重命名**（乱序硬件自动；源码可标量重命名）|
| **输出相关** | A、B 都写 R | 同上 |
| **控制相关** | 是否执行取决于分支 | **if 转换 → select** / 查表 |

> 👉 第 5 课"语句级优化"里 **标量重命名 / 数组重命名**，本质就是在**消这三类相关性**——闭环。

### 3. ILP 武器（代价要记住）

- **循环展开**：给调度器更大原料池 → 更多可并行的指令。⚠️ **代价：寄存器压力↑、代码体积↑、可能溢出到栈反而变慢**。
- **软件流水线**：迭代间重叠"取数/计算/写回"（DSP/VLIW 常用）。

---

## 三、SIMD：向量化 = 一条指令多数据

### 1. 本质
```
标量: sum += a[i]              一次 1 个
SIMD: sum = _mm_add_ps(sum, va)   一次 4 个 (SSE 128位)
AVX2: 一次 8 个 (256 位 ymm)
```

**API 速认**（第 4 课讲过：实战 80% 靠自动向量化，很少手写）：
```c
_mm_load_ps / _mm_loadu_ps   _mm_store_ps / _mm_storeu_ps
_mm_add_ps / _mm_mul_ps / _mm_fmadd_ps   // ⭐ FMA: 一条指令算 a*b+c
_mm_set_ps / _mm_shuffle_ps / _mm_i32gather_ps   // 广播/混洗/聚集
_mm_cmpps / _mm_and_ps / _mm_blendv_ps           // 分支转 select
```

### 2. 五个层次的向量化（对应 `basis/ILP/`）

| 层次 | 做法 | 文件 |
|------|------|------|
| **循环** | 最内层合并成向量（首选）| [`loop_vectorization.cpp`](../../basis/ILP/loop_vectorization.cpp) |
| **基本块** | 块内同构语句打包（非同构先化同构）| [`basic_block_vectorization.cpp`](../../basis/ILP/basic_block_vectorization.cpp) |
| **函数** | 参数/返回都改成向量 | [`function_vectorization.cpp`](../../basis/ILP/function_vectorization.cpp) |
| **分支** | if 转换 + `select` 掩码 | [`branch_vectorization.cpp`](../../basis/ILP/branch_vectorization.cpp) |
| **归约** ⭐ | 归约有依赖，需 `-ffast-math` 或部分和 | [`reduction.cpp`](../../basis/ILP/reduction.cpp) |

### 3. 向量化后 6 大再拧螺丝

| 手法 | 干什么 | 文件 |
|------|--------|------|
| 数据对齐 | 循环剥离头尾 / 数组填充到 4 倍数 | `unaligned_memory_access.cpp` |
| 不连续访存 | `gather` 聚集 / `shuffle` 混洗重组 | `non_contignuous_memory_access.cpp` |
| **向量重用** | 加载一次用两次，别重复 load | `vector_resuse.cpp` |
| **FMA 融合** | mul+add → `_mm_fmadd_ps` 一条 | `vector_operation_fusion.cpp` |
| **完全展开** | 向量后再展开，兼得 DLP+ILP | `loop_unroll.cpp` |
| 不变量合并 | `_mm_set_ps(C,C,C,C)` 提到循环外 | `global_invariant.cpp` |

### 4. ⭐ AoS → SoA：贯穿 SIMD 与访存的杀招

```c
// AoS  粒子挨个存, 想向量化"只处理所有 x"要跨结构体跳步 → 差
struct P { float x,y,z; } particles[N];
// SoA  属性分开存, 所有 x 连续 → 一条 load 拉 8 个 x 向量化 → 好
struct P { float x[N], y[N], z[N]; } ps;
```

**适用判断**（工程总结原话）：
- **AoS** 适合**同时访问同一元素的多个属性**
- **SoA** 适合**批量访问同一属性的多个元素**（向量化更友好）

---

## 四、和 TMA 瓶颈接线（最重要）

```
TMA 诊断出 Core_Bound
   ├─ IPC 低、指令依赖链长  → ILP 手法：打破依赖、展开、重命名
   ├─ 循环没向量化          → SIMD 手法：AoS→SoA、restrict、-ffast-math
   └─ 数据依赖链导致串行累加 → vectorization_2（uint16→uint32 累加器，20.9x！）
                               dep_chains_1（M 路并行链表，3.6x）
```

> **`vectorization_2` 20.9x** 是"消除依赖链"的活广告：原本 16 位窄类型有累加依赖无法并行，换成 32 位独立累加器 → 依赖链断裂 → 向量化彻底释放。

---

## 五、代码实战 · 本机实测

### 实验 ⑧ AoS vs SoA + float vs double（第 4/6 课 SIMD + 第 7 课访存联合）

**基准**：400 万个粒子、每个 `{x,y,z}` 三属性；求所有 x 之和。AoS vs SoA(float) vs SoA(double)。

```bash
# 关键: 归约需 -ffast-math 才允许向量化 (第 4 课)
clang -O3 -march=native -ffast-math -fopenmp -Rpass=loop-vectorize \
      -Rpass-missed=loop-vectorize soa.c -o soa
```

**① 编译器 `-Rpass` 交代**（加 `-ffast-math` 后三者都 "vectorized"）：
```
sum_aos    : vectorized loop (width: 8, interleaved: 4)   ← "看起来都被向量化了"
sum_soa    : vectorized loop (width: 8, interleaved: 4)   ← 8 宽 = AVX2 256 位
sum_soa_d  : vectorized loop (width: 4, interleaved: 4)   ← double 只能塞 4 个
```

**② 实测耗时**（取平均）：

| 布局/类型 | 耗时 | vs AoS |
|---|---|---|
| AoS `struct{float x,y,z;}[N]` | 0.00171 s | — |
| **SoA `float x[]` 连续** | **0.00034 s** | **5.02x** 🔥 |
| SoA `double x[]` | 0.00088 s | 1.94x |

**核心洞察（本课最有价值的一击）**：

> **AoS 明明也被报告"vectorized width 8"，为什么还是慢 5 倍？**
>
> 因为**向量化报告只讲"用没用 SIMD"，不讲"访存有多糟"**。
> - AoS 取 `.x` 是**跨 12 字节的跳步(gather)**；即使向量化，硬件要么走 gather 慢路径，要么加载整段 x,y,z 但只用了 x → **白白读进 3 倍数据、带宽浪费 3×** → 卡在 **Memory Bound（带宽）**。
> - SoA 连续读 `x[i]` → 缓存行 100% 有用 + 干净 SIMD → 5x 快。

**这条实测把第 3 课 TMA 与第 4 课 -Rpass 的分工焊死了**：
> **`-Rpass`（Core 视角）** 会说"AoS 也向量化了没问题"；**`perf` / 计时（Memory 视角）** 揭穿它是带宽瓶颈。**两管必须齐下**。

**double 比 float 慢 2.6x**（0.00088 vs 0.00034）：既因 256 位向量只能装 **4 个 double**（vs 8 个 float）、SIMD 少一半；又因**字节翻倍→带宽减半**。**精度够用就用小类型**——白捡的性能。

---

## 六、一句话收口

> **单核有两种并行**：ILP 靠硬件超标量 + 你不喂依赖；SIMD 靠**自动向量化为主 + 手写 intrinsics 为辅**。实战 80% 是"**把代码写成可向量化形状**"——**AoS→SoA、`restrict` 消别名、消依赖链、紧凑类型**——**"已向量化 ≠ 快"，必须配合访存视角交叉验证**。

## 七、延伸阅读

- 原始详细文档：[`docs/single_core_optimization.md`](../theory/single_core_optimization.md)
- 相关代码：[`basis/ILP/`](../../basis/ILP/)
- 相关实验：`vectorization_1`（AoS→SoA 2.1x）、`vectorization_2`（消依赖 20.9x）、`compiler_intrinsics_1/2`（SSE4.1 / AVX2 手写）、`dep_chains_1`（M 路并行链表）
- 承接：**`07-访存优化.md`**（"喂不饱"才是多数程序真正的病根）
