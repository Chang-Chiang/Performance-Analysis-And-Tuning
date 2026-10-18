# 43 · 实战 · `loop_interchange_2`（高斯模糊）完整闭环

> **TMA 一级分类**：**Backend_Bound / Memory_Bound** · **三级子类**：**TLB Bound** · **加速比**：**9.56x 本机** ≈ 9.7x 文档

---

## 一、读题（Step 0）

**实验目录**：[`TMA/memory_bound/loop_interchange_2/`](../../TMA/memory_bound/loop_interchange_2/)
**核心任务**：对一张 **6856 × 4306** 灰度图（≈29.5 MB）做 1×5 高斯模糊 `{1,4,6,4,1}`，分竖/横两次 1D pass。
**图像文件**：`pexels-pixabay-434334.pbm`（P5 二进制 PGM）· **需命令行传参**：`./lab 输入 输出`

### 工作集大小（决定瓶颈）
```
input  29.5 MB  +  temp  29.5 MB  +  output 29.5 MB
= ~88 MB  ≫ 本机 L3 (36 MB) → 一定 miss 到 DRAM
    每页 4 KB → 全帧 ~7200 页, 但 L2 dTLB 只 ~1536 项 → TLB 装不下
```
👉 **这是"TLB 也吃不下的超大工作集"实验**——**瓶颈不在 cache-miss（那是 DRAM 层的宿命），而在 TLB-miss**（可通过访问模式缓解）。

### 中段代码对照（三段的中间部分，`filterVertically`）

**基线（c 外层、r 内层，每次跳 `width=6856` 字节）**：
```c
for (int c = 0; c < width; c++)                  // ← 外层遍历列
    for (int r = radius; r < height - radius; r++)
        for (int i = 0; i < 5; i++)
            dot += input[(r - radius + i) * width + c] * kernel[i];
            //          ↑ 固定 c, 变 r → 每次跳 6856B, 频繁跨页
```

**优化（r 外层、c 内层连续 + 局部 `dot[width]` 累加）**：
```c
for (int r = radius; r < height - radius; r++) {
    int dot[width];                                 // 27KB 局部数组, 塞进 L1 (48KB)
    for (int i = 0; i < 5; i++)
        for (int c = 0; c < width; c++)
            dot[c] += input[(r - radius + i) * width + c] * kernel[i];
            //       ↑ 固定 r, 变 c → 沿行连续访问, 每 4KB 页覆盖 4 行
    ...
}
```

---

## 二、Step ① 基线 bench + perf

**从零建基线**（工程只提供优化版）：
```bash
cd TMA/memory_bound/loop_interchange_2
cp solution.cpp solution.cpp.opt.bak
# 用 python 脚本把 "Middle part" 换回基线 (c 外层遍历 0..width, r 内层)
cmake -E make_directory build_base && cd build_base
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_FLAGS="-g" -DCMAKE_CXX_FLAGS="-g" ..
cmake --build . --parallel 8
taskset -c 0 ./lab ../pexels-pixabay-434334.pbm /tmp/out_base.pgm
perf stat -e cycles,instructions,cache-references,cache-misses,LLC-load-misses,LLC-loads,dTLB-load-misses \
  taskset -c 0 ./lab ...
```

**Google Bench**：`bench1  128938870 ns   128934629 ns   5 iterations` → **128.94 ms / iter**

**perf 计数**（大核，每 iter）：

| 指标 | 基线合计 | 每 iter |
|------|---------|---------|
| cycles | 4.514 B | 902 M |
| instructions | 3.680 B | 736 M |
| **IPC** | | **0.82** ⚠️ |
| cache-misses / refs (L1) | 15.2M / 18.6M | **82% miss 率** |
| **LLC-load-misses / loads** | 923K / 1.91M | **48% LLC miss 率** |
| **dTLB-load-misses** | 173.2 M | **34.6 M / iter** 🤯 |

**IPC 0.82** = CPU 大片空转；**dTLB miss 每 iter 3460 万次** = 每 21 次访存就一次 TLB miss。

---

## 三、Step ③ 上药（换回紧凑版）+ 复测

```bash
cp solution.cpp.opt.bak solution.cpp
taskset -c 0 ./build/lab ../pexels-pixabay-434334.pbm /tmp/out.pgm
perf stat ...
```

**Google Bench**：`bench1  13488366 ns   13488100 ns   47 iterations` → **13.49 ms / iter**

**加速比 = 128.94 / 13.49 = 9.56x** ✅（文档 9.7x）

| 指标 | 优化每 iter | vs 基线 |
|------|-----------|--------|
| cycles | 102 M | **↓ 8.8x** |
| instructions | 230 M | ↓ 3.2x |
| **IPC** | **2.26** | ↑ 2.8x |
| L1 cache miss 率 | 66% | ↓ 16pp |
| **LLC miss 率** | **50%** | **几乎不变** ⭐ |
| **dTLB-load-misses / iter** | **6.4 K** | **↓ 5418x** 🎯🔥 |

**输出正确性**：`cmp /tmp/out_base.pgm /tmp/out.pgm` → **完全一致** ✓

---

## 四、🎯 加速机制拆解：**不是 SIMD、不是 L3，是 TLB**

本实验的加速比分解公式：
```
cycles/iter ↓ 8.8x  =  instructions/iter ↓ 3.2x  ×  IPC ↑ 2.8x
```

但**真正决定命运的是 dTLB**：

### 为什么基线 TLB 打得这么惨？

固定 `c`、`r` 从 0 扫到 height：
```
每次访存: input[(r-2+i)*width + c]  →  地址相差 6856 字节
```
每 6856 字节 = **每访问就跨 1.7 个 4KB 页**！一整个 `for(r)` 循环下来扫过整张图 29.5MB（7200 页），**远超 dTLB 容量**（L2 dTLB ~1536 项）→ **每几访存就一次 TLB miss**。列 c 变到 c+1 又重来一次，共 6856 次列扫描 → 累计 34.6M dTLB miss/iter。

### 为什么优化版 TLB 几乎消失？

固定 `r`、`c` 从 0 扫到 width：
```
每次访存: input[r*width + c]  →  地址连续 +1
```
沿行连续 6856 字节 → **4 个 4KB 页足够覆盖一整行** → TLB 命中率高得离谱。**整个 88MB 工作集只需 ~22K 页 TLB 记录，且沿行滑动窗口稳定** → 从 34.6M → 6.4K（**5418x**）。

### 关键：`LLC miss 率两版几乎相同（48% vs 50%）`

这个反直觉的观察**是本次实验最重要的收获**：
- 工作集 88 MB **远超 L3 36 MB** → 无论怎么访问，都要 miss 到 DRAM
- 循环交换救不了 DRAM 层的宿命（那是"数据太大"的问题，不是"访问模式"的问题）
- 但**循环交换能救 TLB**（因为 TLB 只跟"页跳转模式"有关，跟数据大小无关）

> 💡 **这就是 TMA 三级下钻的实战价值**：一级 `Memory_Bound`（65%）→ 二级 `L3_Bound vs DRAM_Bound vs TLB_Bound`。**优化要选对层**：
> - **DRAM 层**：只有换数据布局（42 号 data_packing 那种）能救
> - **TLB 层**：换访问顺序（本实验）就够
> - **L1/L2 层**：SIMD/预取更对症

---

## 五、三个 `Memory_Bound` 实验的机制谱系（40/41/42 全景对照）

同为 Memory_Bound 药箱，**瓶颈层次不同 → 药方不同 → 数字表现不同**：

| | **41 loop_interchange_1** | **42 data_packing** | **43 loop_interchange_2** |
|--|---|---|---|
| **TMA 三级子类** | L1/L2 局部性 + **SIMD** | **DRAM Bound**（超 L3）| **TLB Bound**（超 dTLB）|
| 工作集 vs 硬件 | 1.9MB（塞进 L3）| 32MB→8MB（跨 L3 边界）| **88MB（远超 L3 + TLB）** |
| 改的东西 | 代码：循环顺序 | 数据：结构体压缩 | 代码：循环顺序 |
| 核心机制 | 解锁 SIMD → 指令数 ↓10x | 缩到 L3 内 → LLC miss ↓20x | **连续访问跨页数少 → dTLB miss ↓5400x** |
| **IPC 表现** | 4.23→2.73（假降）| 0.99→2.91（真升）| **0.82→2.26**（真升）|
| LLC miss 率变化 | 15.8→0.79%（因基本不落 DRAM）| 15.8→0.79%（跨越边界）| **48%→50%（不变，救不了 DRAM）** |
| dTLB miss 变化 | 小 | 小 | **34.6M→6.4K（5418x）** ⭐ |
| 加速比 | 6.81x | 7.58x | **9.56x** |

> 三实验合起来把 TMA 三级 `Memory_Bound / L1-Bound / L3-Bound / DRAM-Bound / TLB-Bound` **各演示一遍**——是复盘"两大药箱"最好的活标本。

---

## 六、放进 `20 §一` 闭环

| 步骤 | 本实验 |
|------|--------|
| ① 度量基线 | Google Bench 单次 128.94ms |
| ② 定位热点 | `filterVertically` 中段（`c` 外层遍历所有列） |
| ③ TMA 分诊 | IPC 0.82 + dTLB miss **34.6M/iter** → **`Memory_Bound / TLB_Bound`** |
| ④ 开药 | **循环交换 r-outer, c-inner + 局部 `dot[width]` 塞 L1** |
| ⑤ 验证 | 9.56x + dTLB ↓5418x + IPC ↑2.8x = 命中预期 |

---

## 七、一句话收口

> **超大工作集（88MB ≫ L3 36MB）时，DRAM miss 救不了；但 TLB miss 能通过"改访问顺序"救**。这就是**"Memory_Bound 也要分到具体子类再下药"**的最强证据——把 `20 §一 闭环` 和 TMA 三级分诊的价值**同时演示到极致**。加速 **9.56x**、dTLB miss **↓5418x**。

## 八、延伸

- 相关理论：`03-TMA自顶向下分析.md`（三级下钻）、`07-访存优化.md § 七`（TLB 与大页）
- 硬件调优：`03` 里的 `硬件` 章节，Linux 大页 (`Huge Pages`) 能把 4KB 页变 2MB → 直接消灭本例的 TLB 压力
- 原始工程文档：[`docs/loop_interchange_2.md`](../experiments/memory_bound/loop_interchange_2.md)
