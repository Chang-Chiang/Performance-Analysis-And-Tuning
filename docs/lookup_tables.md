# Lookup Tables (Branch Elimination)

> **环境准备**
>
> ```bash
> # 将 CPU 调频策略设为 performance，锁定最高频率，避免动态调频干扰 benchmark 稳定性
> sudo cpupower frequency-set --governor performance
>
> # 创建 build 目录并进入（out-of-source build，保持源码目录干净）
> cmake -E make_directory build && cd build
>
> # 配置构建：Release 模式开启优化（-O2/-O3），同时加 -g 保留调试符号以便 perf 定位源码行
> cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_FLAGS="-g" -DCMAKE_CXX_FLAGS="-g" ..
>
> # 编译，8 线程并行加速构建
> cmake --build . --config Release --parallel 8
> ```

## 背景

对 1M 个随机值（0-150）做直方图分桶，将值映射到 8 个桶（不均匀分布）。

```
桶分布：
  桶 0: 0-12   (13 个值)
  桶 1: 13-28  (16 个值)
  桶 2: 29-40  (12 个值)
  桶 3: 41-52  (12 个值)
  桶 4: 53-70  (18 个值)
  桶 5: 71-82  (12 个值)
  桶 6: 83-99  (17 个值)
  桶 7: ≥100   (默认桶)
```

**问题：** 输入值随机分布，if/else 分支链的条件难以预测，导致大量分支预测失败。

---

## 优化前

### 原始代码

```c++
static std::size_t mapToBucket(std::size_t v) {
  if      (v < 13)  return 0;
  else if (v < 29)  return 1;
  else if (v < 41)  return 2;
  else if (v < 53)  return 3;
  else if (v < 71)  return 4;
  else if (v < 83)  return 5;
  else if (v < 100) return 6;
  return DEFAULT_BUCKET;  // 桶 7
}
```

**问题：** 7 层 if/else 分支链，每次调用最多走 7 次分支判断。输入值随机（0-150），分支预测器无法准确预测走哪条分支。

```
分支预测失败过程：

  v = 47（随机值）
  → v < 13 ?  No  ✗ 预测错误，流水线冲刷
  → v < 29 ?  No  ✗ 预测错误
  → v < 41 ?  No  ✗ 预测错误
  → v < 53 ?  Yes ✓
  → return 3

  每次调用平均 ~3-4 次预测失败
  1M 次调用 × ~3.5 次失败 = ~3.5M 次流水线冲刷
```

### 验证正确性

```shell
$ ./validate
Validation Successful
```

### 运行 benchmark

```shell
$ cmake --build . --target benchmarkLab
-----------------------------------------------------
Benchmark           Time             CPU   Iterations
-----------------------------------------------------
bench1           3939 us         3929 us          710
```

### Profile

一级分析（Top-Down）：

```shell
$ perf stat --topdown -a taskset -c 0 ./lab
-----------------------------------------------------
Benchmark           Time             CPU   Iterations
-----------------------------------------------------
bench1           4126 us         4025 us          174

 Performance counter stats for 'system wide':

 %  tma_backend_bound %  tma_retiring %  tma_frontend_bound %  tma_bad_speculation
                30.4           15.3                16.8                   50.2
```

二级分析（toplev.py L2）：

```shell
$ toplev.py --core S0-C0 -l2 --no-desc taskset -c 0 ./lab
core BAD              Bad_Speculation                     % Slots                       60.0  <==
core BAD              Bad_Speculation.Branch_Mispredicts  % Slots                       60.0
```

**瓶颈分析：** Bad Speculation 高达 60.0%——CPU 执行槽中有 60% 的时间在处理被错误预测的指令。每次分支预测失败需要冲刷流水线 ~15-20 个周期，1M 次调用 × ~3.5 次失败 = 浪费大量时间。

---

## 优化后

### 优化后代码

用查找表替代 if/else 分支链，将分支判断转为数组下标访问：

```c++
// 预计算查找表：100 个元素，每个元素直接存储桶号
static const int buckets[100] = {
    0,0,0,0,0,0,0,0,0,0, 0,0,0,                         // 0-12  → 桶 0
    1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,                   // 13-28 → 桶 1
    2,2,2,2,2,2,2,2,2,2, 2,2,                            // 29-40 → 桶 2
    3,3,3,3,3,3,3,3,3,3, 3,3,                            // 41-52 → 桶 3
    4,4,4,4,4,4,4,4,4,4, 4,4,4,4,4,4,4,4,               // 53-70 → 桶 4
    5,5,5,5,5,5,5,5,5,5, 5,5,                            // 71-82 → 桶 5
    6,6,6,6,6,6,6,6,6,6, 6,6,6,6,6,6,6                  // 83-99 → 桶 6
};

static std::size_t mapToBucket(std::size_t v) {
  if (v < sizeof(buckets) / sizeof(buckets[0]))
    return buckets[v];   // 直接数组访问，无分支
  return DEFAULT_BUCKET; // ≥100 → 桶 7
}
```

**改进点：** 7 层 if/else → 1 次边界检查 + 1 次数组访问。查找表 400B（100×4B），完全在 L1 缓存中。

### 验证正确性

```shell
$ ./validate
Validation Successful
```

### 运行 benchmark

```shell
$ cmake --build . --target benchmarkLab
-----------------------------------------------------
Benchmark           Time             CPU   Iterations
-----------------------------------------------------
bench1           2103 us         2102 us         1279
```

### Profile

一级分析（Top-Down）：

```shell
$ perf stat --topdown -a taskset -c 0 ./lab
-----------------------------------------------------
Benchmark           Time             CPU   Iterations
-----------------------------------------------------
bench1           2112 us         2111 us          328

 Performance counter stats for 'system wide':

 %  tma_backend_bound %  tma_retiring %  tma_frontend_bound %  tma_bad_speculation
                26.2            8.8                33.3                   42.7
```

二级分析（toplev.py L2）：

```shell
$ toplev.py --core S0-C0 -l2 --no-desc taskset -c 0 ./lab
core FE               Frontend_Bound                      % Slots                       31.8
core BAD              Bad_Speculation                     % Slots                       47.1
core BAD              Bad_Speculation.Branch_Mispredicts  % Slots                       47.1  <==
```

---

## 优化分析

### 性能对比

| 指标 | 优化前（if/else） | 优化后（查找表） | 变化 |
|------|------------------|----------------|------|
| benchmark 耗时 | 3939 μs | 2103 μs | **1.87x 加速** |
| Bad Speculation | 60.0% | 47.1% | **-12.9%** |
| Frontend Bound | — | 31.8% | — |

### 为什么有效

1. **分支减少**：7 层 if/else → 1 次边界检查。每次调用的分支预测失败从 ~3.5 次降至 ~1 次。

2. **Bad Speculation 下降（60.0% → 47.1%）**：查找表消除了 6 条分支，仅保留 `v < 100` 的边界检查。剩余的 47.1% 主要来自循环分支和边界检查。

3. **Frontend Bound 上升**：查找表访问需要额外的内存加载指令（`buckets[v]`），但这个代价远低于分支预测失败的代价。

### 查找表 vs 分支链

```
分支链 (7 次比较)：
  v=47: 13? → 29? → 41? → 53? → return 3  (4 次分支，~3 次预测失败)

查找表 (1 次比较 + 1 次加载)：
  v=47: v < 100? → buckets[47] → return 3   (1 次分支，0-1 次预测失败)
```

### 为什么 Bad Speculation 仍然有 47.1%

1. **循环分支**：`for (auto v : values)` 每次迭代都有循环条件判断，虽然预测准确率高（几乎总是继续），但 1M 次迭代仍有少量预测失败。

2. **边界检查**：`if (v < 100)` 对于 v ≥ 100 的值（~33% 概率）仍可能预测失败。

3. **可能的进一步优化**：用无分支的查找表（如 `_mm_cmplt_epi32` + 位运算）可以完全消除分支，但代码复杂度增加。
