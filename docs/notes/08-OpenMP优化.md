# 08 · OpenMP 程序优化

> **立意**：从"榨干一个厨师"到"**多厨师共用一间厨房**"（共享内存多核）。算力上去了，但冒出**一堆全新的开销**——Fork-Join、负载不均、伪共享、同步等待。**OpenMP 优化的本质：在"多开线程"和"控制这些新开销"之间找平衡。**

---

## 一、OpenMP 是什么：贴标签就能并行

Linux 自带、`#include <omp.h>` + `gcc -fopenmp`。

```c
#pragma omp parallel for private(j,k) shared(A,B,C) num_threads(4)
for (i = 0; i < N; i++)          // ✓ 并行最外层(迭代最多)
    for (j = 0; j < N; j++)
        for (k = 0; k < N; k++)
            C[i][j] += A[i][k]*B[k][j];
```
编译运行：`gcc -O2 -fopenmp app.c && OMP_NUM_THREADS=8 ./app`

**执行模型 = Fork-Join**：
```
主线程 ──串行──┐
              ├─ Fork：并行区开始, 生成 T 个线程
              │  多线程并行
              ├─ Join：并行区结束, 栅障(barrier)同步
串行代码 ─────┘
```
> 👉 **每次进/出并行区都要创建/合并线程**——这是"并行区扩张/合并"要消灭的对象。**厨房类比**：每炒一个菜就把厨师招来又辞退，光办入职离职就累死。

**入门代码**：[`serial_example.c`](../../basis/OpenMP/Basics/serial_example.c) → [`first_openmp_program.c`](../../basis/OpenMP/Basics/first_openmp_program.c) → [`matrix_multiply_parallel.c`](../../basis/OpenMP/Basics/matrix_multiply_parallel.c)

---

## 二、⭐ 头号坑：变量作用域（shared / private / reduction）

多厨师共用台面——**哪些公共、哪些各自一份**必须说清，否则算错：

| 子句 | 含义 | 类比 |
|------|------|------|
| `shared(x)` | 所有线程**共用一份** | 公共砧板 |
| `private(x)` | 每线程**各一份独立**（未初始化）| 每人自己的碗 |
| `firstprivate(x)` | 各一份，**带入初值** | 拷贝了原始配方 |
| `reduction(+:sum)` ⭐ | 各线程私有累加，**结束时自动合并** | 各记各的账，最后对账 |

**`reduction` 是最大杀器**：`sum += ...` 这类归约有循环依赖（第 6 课），标量下没法向量化/并行，但 `reduction(+:sum)` **既并行又正确**（每线程寄存器私有累加 + 末尾合并）——**还顺带解锁向量化**（呼应实验⑧）。

---

## 三、⭐ 三大"并行专属开销"及对策（本课骨架）

`basis/OpenMP/` 各子目录一一对应：

### 开销 ①：Fork-Join → **并行区扩张 & 合并**（`ParallelRegion/`）
```c
// ✗ 并行在里层: 每次 i 迭代都 fork-join 一遍
for (i) { #pragma omp parallel for \n for (j) ... }
// ✓ 扩张到外层: fork-join 从 N 次 → 1 次
#pragma omp parallel for \n for (i) for (j) ...
```
代码：[`parallel_region_expansion.c`](../../basis/OpenMP/ParallelRegion/parallel_region_expansion.c)、[`parallel_region_merging.c`](../../basis/OpenMP/ParallelRegion/parallel_region_merging.c)

### 开销 ②：负载不均 → **`collapse` + `schedule` 调度**（`LoadBalancing/`）

**`collapse(n)`**：把相邻 n 层循环合并成大迭代空间再分配 → 分工更细更匀。

**`schedule` 策略**（背这张表）：

| 策略 | 机制 | 适用 |
|------|------|------|
| `static` | 提前**均分**块，开销最低 | 每迭代耗时**相近**（规则循环，如矩阵乘）|
| `dynamic` | **先到先得**领块 | 每迭代耗时**差异大**（随机型）|
| `guided` | 块**由大到小**递减 | 耗时**递减型** |
| `runtime` | 环境变量 `OMP_SCHEDULE` 定 | 不重编译就调策略 |

> 本质：**负载均衡 vs 调度开销的权衡**。多数规则循环默认 `static` 就够。

代码：[`collapse_directive.c`](../../basis/OpenMP/LoadBalancing/collapse_directive.c)、`schedule_{static,dynamic,guided}.c`

### 开销 ③：同步等待 → **隐式 barrier + `nowait`**（`nowait_clause.c`）
`for`/`sections` 结束**自动加一道 barrier**（所有线程到齐才继续）→ 快线程干等。
```c
#pragma omp for nowait   // 去掉这道 barrier，先干完的接着干
```
⚠️ `nowait` 的安全前提：后续代码**不依赖**这个循环的结果。含 `reduction`/`ordered`、用了 `dynamic`/`guided` 时**不能乱加**。

---

## 四、伪共享 / 向量化指导 / 线程数

**伪共享**：`basis/OpenMP/FalseSharing/`（详见 §五 实验⑥）——多线程各写 `result[id]`，同一 64B 缓存行争用。解法：padding `result[T][8]` 或 `reduction(+:result)`（**优先 reduction**：简洁又省内存）。

**向量化指导**：`basis/OpenMP/Vectorization/`——OpenMP 把 SIMD 做成开关：
```c
#pragma omp simd                 // 单线程内向量化
#pragma omp for simd simdlen(8)  // 多线程 for + 线程内 SIMD
```
验证：`gcc -O3 -fopenmp -fopt-info-vec-optimized` 或 clang `-Rpass=loop-vectorize`（第 4 课）。

**线程数**（Amdahl 回声）：
- **一般 ≤ 物理核心数**（超线程资源竞争）
- 小任务并行反而亏 → `if(N>阈值)` 自动串/并切换 → [`if_clause.c`](../../basis/OpenMP/LoadBalancing/if_clause.c)
- **线程数扫描找拐点**是标准调优动作（见实验⑦）

---

## 五、代码实战 · 本机实测

### 实验 ⑥ 伪共享 + 一次真实的"翻车"（`basis/OpenMP/FalseSharing/`）

**第一次跑**（工程原代码，`NTHREADS=4, num_steps=10⁶`）：
```
串行 0.000884s | 伪共享 0.000576s | 填充 0.001031s     ← "反了"!
```

**诊断**：整段 < 1ms → OpenMP **启动线程的 fork/join 开销**（几十~上百μs）+ 系统噪声直接**盖过**了争用效应；单次亚毫秒计时抖动比被测效应还大。

**修正方法**（第 1 课 + 30 手册纪律）：
1. **规模放大** num_steps 10⁶ → **2×10⁷**（效应积累）
2. **线程 = 8**（8×double=64B **正好填满一整条缓存行**，争用最狠）
3. **绑核** `OMP_PROC_BIND=spread OMP_PLACES=cores`（消调度噪声）
4. **两次跑** 确认排序稳定

**修正后**：
```
Run1: 伪共享 0.0038 > 填充 0.0026 ≈ reduction 0.0026
Run2: 伪共享 0.0027 > reduction 0.0024 > 填充 0.0023   ✅ 稳定
```

**机制**（对应 `07 § 三` MESI）：`sum[8]` 全在一条缓存行，线程 0 写 `sum[0]` → 该行的独占权在线程间**疯狂乒乓（RFO）**；明明"各写各的、逻辑无关"，物理上被强行串行。padding 让每线程独占一行、reduction 让每线程寄存器私有累加 → 都消除争用。

> 👉 **本课最重要的元教训**：**微基准第一次跑出"反直觉结果"，先怀疑测量，别急着怀疑计算机**。加速比只有 ~1.2-1.5x（浮点除法本身重，部分稀释争用；本机单路共享 L3；若是多插槽 NUMA 会灾难性地慢）。这提醒：**加速比强依赖硬件与负载，换机器就要重测**。

### 实验 ⑦ OpenMP 加速比曲线（本机 24 逻辑核）

1200×1200 矩阵乘，i-k-j 连续访问 + `-O3 -march=native`，多次平均：

| 线程 | 实测(s) | 加速比 | 效率 E=S/P |
|---|---|---|---|
| 1 | 0.120 | 1.00x | 100% |
| 2 | 0.066 | 1.83x | 92% |
| 4 | 0.033 | 3.64x | 91% |
| 8 | 0.019 | 6.44x | 81% |
| **12** | 0.012 | **9.97x** | 83% |
| 16 | 0.011 | 10.67x | **67%** ↓ |
| 24 | 0.010 | 12.17x | **51%** ↓↓ |

**读曲线**：
1. **前段近线性**（1→12 线程，效率稳在 90%+）：因为**纯计算密集 + 连续访存 + 已向量化**（前 7 课的先修）→ 并行才铺得开。
2. **后段急转封顶**（16→24 几乎不动，效率 90%→51%，**卡在 ~12x**）：
   - **Amdahl 串行天花板**（第 1 课）：init、fork/join、每线程读共享 B 的行
   - **物理核用完**：本机大约 12 个物理核，16/24 靠**超线程**硬凑 → 边际收益骤降
   - **内存带宽墙**（第 7 课）：24 线程抢同一条内存总线给 B 喂数据 → Memory_Bound 抬头

**工程启示**：**线程数别盲目拉满**，通常取**物理核数**最划算；用 `OMP_NUM_THREADS` 扫一遍找拐点（就像上面），是调 OpenMP 的标准动作。

---

## 六、"加了线程不加速"排查清单（工程血泪）

按优先级：
1. **Fork-Join 吃掉收益** → 循环太小 / 并行在里层 → 扩张合并、`if` 串行回退
2. **负载不均** → 迭代耗时差异大 → 换 `schedule(dynamic)` 或 `collapse`
3. **伪共享** → 多线程写共享数组同缓存行 → `perf c2c` 查，改 padding/reduction
4. **隐式同步** → barrier 过多 → 试 `nowait`（安全性看 §三 ③）
5. **Amdahl 串行段** → 归约/IO 主导 → 用 `reduction`、看 VTune Threading 空闲比
6. **撞物理核上限** → 用 §五 实验⑦ 的扫描法定拐点
7. **测量本身不可信** → 见 `30 测量纪律手册`：噪声 / 未绑核 / 规模太小 / 未初始化变量

---

## 七、一句话收口

> **并行不是免费午餐**：每加一个 `#pragma` 都要算"并行收益 vs 协调开销"。**头号坑是变量作用域**（`shared/private/reduction`）；**三大开销**（fork-join / 负载不均 / 同步等待）分别对应"扩张合并 / collapse+schedule / nowait"；**伪共享是并行程序"加了核反而慢"的头号元凶**（padding/reduction/perf c2c）；**线程数 ≤ 物理核心**是 Amdahl 的直接体现。

## 八、延伸阅读

- 原始详细文档：[`docs/openmp_optimization.md`](../theory/openmp_optimization.md)、[`docs/false_sharing.md`](../experiments/memory_bound/false_sharing.md)
- 相关代码：[`basis/OpenMP/`](../../basis/OpenMP/)
- 相关实验：[`TMA/memory_bound/false_sharing_1/`](../../TMA/memory_bound/false_sharing_1/)（alignas(64) 消伪共享 **15.8x**）
- 承接：**`09-CUDA优化.md`**（把"多厨师"变成"上千学徒"，思维再大跳跃）
