# 42 · 实战 · `data_packing`（结构体压缩）完整闭环

> **TMA 一级分类**：**Backend_Bound / Memory_Bound** · **优化技术**：结构体压缩（位域 + float）· **加速比**：**7.58x 本机** ≈ 7.5x 文档

---

## 一、读题（Step 0）

**实验目录**：[`TMA/memory_bound/data_packing/`](../../TMA/memory_bound/data_packing/)
**核心代码**：`solution.h` 只定义了一个数据结构 + `solution.cpp` 做**打乱 + 计数排序**（`N=10⁶` 元素，key=`v.i` ∈ [0,100]）。**排序算法两版完全一致，唯一改的是结构体的"摆放方式"**。

### 基线宽版（32B）vs 优化紧凑版（8B）

```cpp
// 基线: 每个字段独立, 有 padding
struct S { int i; long long l; short s; double d; bool b; };
// → sizeof(S) ≈ 32 字节 (字段和 = 23B, 加上按 double 8B 对齐的 padding)

// 优化: 位域打包, d 从 double 降为 float
struct S { float d; long long l:16; int i:8; unsigned short s:7; bool b:1; };
// → sizeof(S) = 8 字节: float 4B + 位域 (16+8+7+1=32 bits=4B)
```

**关键差异**：`N = 10⁶` 时
- 基线工作集 = `10⁶ × 32B = 32 MB` → **超出本机 L3（~30 MB）边界**
- 优化工作集 = `10⁶ × 8B = 8 MB` → **整个塞进 L3**

**这就是"数据布局 = Memory_Bound 生死线"最纯粹的实验**。

---

## 二、Step ①② 基线 bench + perf

**从零建基线**（工程只预构建了紧凑版）：
```bash
cd TMA/memory_bound/data_packing
cp solution.h solution.h.opt.bak        # 备份紧凑版
# 把 solution.h 里"基线宽版" struct 启用 (原注释里)
cmake -E make_directory build_base && cd build_base
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_FLAGS="-g" -DCMAKE_CXX_FLAGS="-g" ..
cmake --build . --parallel 8
./validate                                # Validation Successful ✓

# 基线 bench (Google Bench 自适应 22 iters)
taskset -c 0 ./lab                        # → 31.7 ms
perf stat -e cycles,instructions,cache-references,cache-misses,LLC-load-misses,LLC-loads taskset -c 0 ./lab
```

**Google Bench**：`bench1   31.7 ms   31.7 ms   22 iterations`

**perf 计数**（大核视角 99.97%）：

| 指标 | 基线合计 | 基线/iter |
|------|---------|-----------|
| cycles | 5.95 B | 270 M |
| instructions | 5.88 B | 267 M |
| **IPC** |  | **0.99** |
| cache-references | 217 M | 9.9 M |
| cache-misses | 97.9 M | 4.45 M |
| **cache miss 率** |  | **45%** ⚠️ |
| LLC-loads | 45.6 M | 2.1 M |
| LLC-load-misses | 7.22 M | 328 K |
| **LLC miss 率** |  | **15.8%** 🔥 |
| sys 时间 | 0.37 s | — （page fault 高）|

**IPC 0.99 + LLC miss 15.8%** = **教科书级 Memory_Bound**：CPU 每周期只退休 1 条指令、每 6 次 LLC load 就 miss 一次去 DRAM。

---

## 三、Step ③④ 上药（换回紧凑版）+ 复测

```bash
cd ..
cp solution.h.opt.bak solution.h
taskset -c 0 ./build/lab                  # 已有的紧凑版预构建
perf stat ... taskset -c 0 ./build/lab
```

**Google Bench**：`bench1   4.18 ms   4.18 ms   164 iterations`

**加速比 = 31.7 / 4.18 = 7.58x** ✅

**perf 计数**（大核视角）：

| 指标 | 优化合计 | 优化/iter | vs 基线/iter |
|------|---------|----------|-------------|
| cycles | 6.51 B | 40 M | **↓ 6.75x** |
| instructions | 18.93 B | 115 M | **↓ 2.32x** |
| **IPC** |  | **2.91** | **↑ 2.9x** 🔥 |
| cache-misses | 10.1 M | 62 K | ↓ 72x |
| **cache miss 率** |  | **2.9%** | ↓ 16x |
| LLC-load-misses | 0.76 M | 4.6 K | ↓ 71x |
| **LLC miss 率** |  | **0.79%** | **↓ 20x** ⭐ |
| sys 时间 | 0.02 s | — | ↓ 18x（page fault 少） |

---

## 四、🎯 加速机制拆解

**加速 7.58x 是两个因素相乘**：

```
cycles/iter ↓ = (instructions/iter ↓) × (1/IPC ↑)
       6.75x  =        2.32x           ×   2.9x
```

1. **指令数少 2.3x**：紧凑布局下 load/copy 操作搬运的字节少 → 相同拷贝工作量下**指令数直接砍**（第 7 课"紧凑类型 = 缓存装更多 + SIMD 更宽"的量化）
2. **IPC 提升 2.9x**：LLC miss 率从 15.8% → 0.79%，CPU 不再天天等 DRAM → **流水线填满**（第 2/3 课核心指标的实物）
3. 两个乘起来 ≈ 6.75x cycles 减少 → 31.7ms/4.18ms = **7.58x**（差异部分来自 sys/page-fault 开销）

**LLC miss 率掉 20x 是最"根因级"的数字**——它精确对应"32 MB 超 L3 → 8 MB 塞进 L3"这个**容量边界的跨越**。工作集 32MB 时**每 6 次 LLC load 就 miss 去 DRAM 一次**，紧凑后**每 125 次才 miss**。

> 💡 **换个视角**：`sys 时间从 0.37s 降到 0.02s（18x）`——这是 page fault。32 MB 工作集要 8192 次 4KB 页故障，8 MB 只要 2048 次。第 1 课说 `sys 高 = 系统调用/缺页`，这个实验是最直白的印证。

---

## 五、放进 `20 §一` 闭环对照

| 步骤 | 本实验 |
|------|--------|
| ① 度量基线 | Google Bench 单次 31.7ms |
| ② 定位热点 | 整个 solution() 就是热点（打乱+计数排序） |
| ③ TMA 分诊 | **IPC 0.99 + LLC miss 15.8% = 典型 Memory_Bound（DRAM 层）** |
| ④ 开药 | **结构体压缩：位域 + float，32B→8B**（第 7 课紧凑类型杠杆） |
| ⑤ 验证 | **7.58x 加速 + LLC miss 率↓20x + IPC↑2.9x = 命中预期** |

---

## 六、和 41 号实验（loop_interchange_1）的**对比**

同为 Memory_Bound 药箱，但**收益来源不同**：

| | 40 loop_interchange | 42 data_packing |
|--|-------------------|----------------|
| 改了什么 | **代码**：循环顺序 i-j-k → i-k-j | **数据**：结构体宽版 → 紧凑版 |
| 主要机制 | 解锁 SIMD 自动向量化（指令数 ↓10x） | 工作集塞进 L3（**LLC miss 率 ↓20x**） |
| IPC 变化 | 4.23 → 2.73（假降，实际逻辑吞吐 ↑5.2x）| 0.99 → 2.91（真升）|
| 加速比 | 6.81x | 7.58x |

**两实验并列**能完整说明 Memory_Bound 优化药箱的**两个不同侧面**：**代码摆对访问顺序**（40）+ **数据摆对布局大小**（41）。

---

## 七、复现命令清单

```bash
cd TMA/memory_bound/data_packing
cp solution.h solution.h.opt.bak
# 用宽版 struct 覆盖 solution.h (注释里已有)
cmake -E make_directory build_base && cd build_base
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_FLAGS="-g" -DCMAKE_CXX_FLAGS="-g" ..
cmake --build . --parallel 8 && ./validate
taskset -c 0 ./lab
perf stat -e cycles,instructions,cache-references,cache-misses,LLC-load-misses,LLC-loads taskset -c 0 ./lab
cd ..
cp solution.h.opt.bak solution.h
taskset -c 0 ./build/lab
perf stat ... taskset -c 0 ./build/lab
```

---

## 八、一句话收口

> **纯 Memory_Bound 教学标本**：**代码一行没改、只把结构体从 32B 压到 8B**，就跨过了 L3 容量边界 → LLC miss 率 15.8%→0.79%（↓20x）、IPC 0.99→2.91（↑2.9x）、加速 **7.58x**。这就是第 7 课"**紧凑类型 = 让缓存装下更多 = 治 DRAM Bound**"最纯的量化。

## 九、延伸

- 相关理论：`07-访存优化.md § 四/六`（结构体对齐 + 紧凑类型）
- 相关工具：`02-性能分析工具.md § 二.2`（`perf stat` 的 LLC 事件）
- 同类实验：`41-实战-loop_interchange_1.md`（同为 Memory_Bound，但机制不同——两相对照更完整）
- 原始工程文档：[`docs/data_packing.md`](../experiments/memory_bound/data_packing.md)
