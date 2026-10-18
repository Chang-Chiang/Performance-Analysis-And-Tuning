# 03 · TMA 自顶向下微架构分析

> **立意**：`perf stat` 给你 IPC / cache-miss 的**模糊指标**还不够——TMA 把"为什么慢"精确下钻到"是 L1 慢还是 DRAM 慢、是执行单元饱和还是分支预测错"。**本仓库 `TMA/` 目录所有实验的方法论根基。**

**TMA = Top-Down Microarchitecture Analysis**（Intel 提出）。核心思想：**别一上来扎进具体事件，先按四大类分诊，再逐层往下钻。**

---

## 一、CPU 厨房比喻（把抽象概念坐实）

- **前端(Front-End)**：取指 + 解码 = 查菜谱/备料
- **后端(Back-End)**：执行指令 = 起锅炒菜
- **流水线槽 (pipeline slot)** = **一个核在一个周期内所能发射一条 μOP 的位置**（Intel PMU 里的最小可观察单元）

一个槽只会处于 **4 种状态之一**（**注意是"槽"的状态**，不是指令）：

| 状态 | 情形 | 厨房类比 |
|------|------|---------|
| ✅ **Retiring** | 槽用上，成功发射并退休 μOP | 出菜上桌 |
| ❌ **Bad Speculation** | 槽用上，但 μOP 后来被作废 | 炒一半发现点错单，整盘倒掉 |
| 🍽️ **Frontend Bound** | 槽空着：前端没喂来 μOP | 灶冷着，配菜没送到 |
| 🔥 **Backend Bound** | 槽空着：后端资源不足收不下 | 料到了但灶台/厨师满了 |

> **对称结构**（务必记）：`Retiring` + `Bad Speculation` = **槽被占用**（发射了）；`Frontend` + `Backend Bound` = **槽被浪费**（空转）。目标是拉高 Retiring、压低其他三个。
> 四者相加 = 100%，所以 `perf stat --topdown` 输出是四个百分比。

---

## 二、三层下钻：决策树（本课灵魂）

```
perf stat --topdown   →  谁占比最高？
   ├─ Retiring 高            → 性能好，别乱优化 ✅
   ├─ Bad Speculation 高     → 分支预测问题 → 查表 / 平衡判断树
   ├─ Frontend Bound 高      → 下钻: i-Cache Miss / iTLB Miss / Branch Resteer / Fetch Bandwidth
   └─ Backend Bound 高  ← 最常见
         ├─ Memory_Bound → 下钻 L1/L2/L3/DRAM/Store Bound → 访存优化
         └─ Core_Bound   → 下钻 Divider/依赖链/端口饱和     → 向量化 / ILP
```

### 三大瓶颈 → 药方对应表（背下来）

| 瓶颈类型 | 病因 | 药方 |
|---------|------|------|
| **Bad Speculation** | 分支预测老错 | 查找表替代 if-else 链（→ `lookup_tables_1`）|
| **Memory Bound** | 数据喂不上 | 循环交换/分块、紧凑布局/AoS→SoA、软件预取、alignas 消伪共享 |
| **Core Bound** | 算力/依赖堵 | SIMD 向量化、打破依赖链、函数内联、FMA |

---

## 三、命令链：从一级到代码定位

```bash
# 一级：粗分四大类
perf stat --topdown -a -- taskset -c 0 ./benchmark
# Retiring: 25% | Bad Spec: 2% | Frontend: 8% | Backend: 65%

# 二级：把 Backend 拆成 Memory vs Core
toplev -l2 --core S0-C0 -- ./benchmark
# Backend_Bound.Memory_Bound: 52% | Core_Bound: 13%

# 三级：Memory 拆到缓存层级
toplev -l3 --core S0-C0 -- ./benchmark
# Memory_Bound.L3_Bound: 38% | DRAM_Bound: 14%

# 定位到代码行：精确采样该事件
perf record -e MEM_LOAD_RETIRED.L3_MISS ./benchmark
perf report
```

**四步链路**：`perf stat --topdown` → `toplev -l2` → `toplev -l3` → `perf record -e 具体事件`。

## 四、VTune 中的等价流程（GUI 版）

1. **Performance Snapshot** → 记录运行时间作基线
2. **Hotspots** → 定位最耗时函数/语句 + CPU 利用率直方图
3. **Microarchitecture Exploration (ue)** → **底层就是 TMA**，GUI 里直接展开决策树
4. 依 TMA 结果分类下钻，回到 Hotspots 定位到代码行

⚠️ **铁律**：分析时程序要**生产同款编译（`-O2/3`）**，加 `-g` 让工具能定位到源码行。**用 `-O0` 分析瓶颈是假的**——因为编译器根本没优化。

---

## 五、代码实战 · 本机实测

### 实验 ⑤ 循环交换 + perf 硬件计数器（`basis/Measure/loop_interchange_{before,after}.c`）

**同一矩阵乘、仅换循环顺序，用 `perf stat` 挖底层原因**：

```bash
gcc -O2 loop_interchange_before.c -o li_before   # i-j-k, z[k][j] 列跳步
gcc -O2 loop_interchange_after.c  -o li_after    # k-i-j, z[k][j] 行连续
perf stat -e cache-references,cache-misses,instructions,cycles ./li_{before,after}
```

n=1024 双精矩阵乘（大核 `cpu_core` 视角，本机 98%/83% 时间在大核）：

| 指标 | before | after | 变化 |
|------|--------|-------|------|
| 运行时间 | 1.930 s | 0.203 s | **9.5x 加速** |
| cache-references | 7.9 亿 | 0.63 亿 | **↓ ~12x** |
| instructions | 75.2 亿 | 70.3 亿 | 几乎不变 |
| cycles | 110.2 亿 | 11.6 亿 | ↓ ~9.5x |
| **IPC** | **0.68** | **6.04** | **↑ ~9x** 🔥 |

**读表**：
- 指令数几乎不变（**算法没改**），**cycles 少了 9.5 倍** → 全部差异都在 **IPC**。
- **before IPC 0.68** ≪ 大核 4-6 发射宽度 → **CPU 大片空转，在等内存** = TMA 一级诊断的 **Backend_Bound.Memory_Bound**。
- **after IPC 6.04**（甚至超宽，因为触发 SIMD 一次多路）→ 后端被喂饱。
- **cache-references 少 12x** = 行连续访问时，一条 64B 缓存行可给内层 16 个 int 复用；列访问则每跨行都要取一条新缓存行。

> 👉 这一组数字，把第 1 课 Roofline 的"访存瓶颈区"、第 2 课 IPC 直觉、第 3 课 TMA Memory_Bound 三者**焊死**：同样指令数下，仅让访问连续，IPC 从 0.68 飙到 6.04。

**对应实验**：[`TMA/memory_bound/loop_interchange_1/`](../../TMA/memory_bound/loop_interchange_1/)（6.9x）、[`loop_interchange_2/`](../../TMA/memory_bound/loop_interchange_2/)（9.7x）；官方 Roofline 对比见 [`docs/intel_advisor.md`](../theory/intel_advisor.md)。

---

## 六、一句话收口

> **TMA 是"先分诊再下药"的框架**：拿到慢程序 → 先 `perf stat --topdown` 归 4 类 → 二级拆 Backend 到 Memory vs Core → 三级下钻到具体缓存层/依赖链 → `perf record -e 事件` 落到代码行。**开错药比不开药更浪费**（给 Memory_Bound 上向量化 = 白费力气）。

## 七、延伸阅读

- 原始详细文档：[`docs/tma.md`](../theory/tma.md)、[`assets/TMA_hierarchy.jpeg`](../../assets/TMA_hierarchy.jpeg)
- VTune 使用：[`docs/performance_analysis.md`](../theory/performance_analysis.md)
- Intel 官方 PDF：[`Software Optimizations Become Simple with Top-Down Analysis Methodology.pdf`](../reference/Software%20Optimizations%20Become%20Simple%20with%20Top-Down%20Analysis%20Methodology.pdf)
- 承接：**`04-编译器概述.md`** 进入"单核药箱"的具体配方
