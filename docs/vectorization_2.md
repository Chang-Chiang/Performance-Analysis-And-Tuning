# Vectorization (Checksum)

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

对 64K 个 `uint16_t` 元素求 checksum（16 位累加和，带进位处理）。

```
数据：Blob = std::array<uint16_t, 64*1024>  ← 128KB
```

---

## 优化前

### 原始代码

```c++
uint16_t checksum(const Blob &blob) {
  uint16_t acc = 0;
  for (auto value : blob) {
    acc += value;
    acc += acc < value; // add carry
  }
  return acc;
}
```

**问题：** `acc += acc < value` 是进位检查——每次迭代都依赖上一次的 `acc` 值，形成严格的循环依赖链。编译器无法向量化这种依赖循环。

```
迭代依赖链：
  acc[0] = 0 + blob[0] + carry_check
  acc[1] = acc[0] + blob[1] + carry_check  ← 依赖 acc[0]
  acc[2] = acc[1] + blob[2] + carry_check  ← 依赖 acc[1]
  ...完全串行，无法并行
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
bench1           24.3 us         24.3 us       115207
```

### Profile

一级分析（Top-Down）：

```shell
$ perf stat --topdown -a taskset -c 0 ./lab
-----------------------------------------------------
Benchmark           Time             CPU   Iterations
-----------------------------------------------------
bench1           24.3 us         24.3 us        28805

 Performance counter stats for 'system wide':

 %  tma_backend_bound %  tma_retiring %  tma_frontend_bound %  tma_bad_speculation
                29.7           23.4                 5.1                    0.7
```

二级分析（toplev.py L2）：

```shell
$ toplev.py --core S0-C0 -l2 --no-desc taskset -c 0 ./lab
core BE               Backend_Bound             % Slots                       74.9
core BE/Core          Backend_Bound.Core_Bound   % Slots                       74.9  <==
```

**瓶颈分析：** Core Bound 74.9%——CPU 执行单元被依赖链阻塞。每次 `acc += value` 后必须等结果出来才能做 `acc += acc < value`，流水线无法提前发射下一次迭代的指令。

---

## 优化后

### 优化后代码

将累加器从 `uint16_t` 拓宽为 `uint32_t`，消除循环内的进位检查，循环结束后再做一次进位修正：

```c++
uint16_t checksum(const Blob &blob) {
  // 拓宽累加器：64K × 65535 = 4.29B < 2^32，不会溢出
  uint32_t acc = 0;
  for (const auto value : blob) {
    acc += value;  // 无进位检查，无依赖链！
  }

  // 循环结束后一次性进位修正
  auto high = acc >> 16;
  auto low = acc & 0xFFFFu;
  acc = low + high;

  high = acc >> 16;
  low = acc & 0xFFFFu;
  acc = low + high;

  return static_cast<uint16_t>(acc);
}
```

**改进点：** 循环内只剩 `acc += value`，无依赖链——编译器可以将多个 `acc += value` 拆开并行执行（向量化），最后再合并结果。

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
bench1           1.16 us         1.16 us      2421126
```

### Profile

一级分析（Top-Down）：

```shell
$ perf stat --topdown -a taskset -c 0 ./lab
-----------------------------------------------------
Benchmark           Time             CPU   Iterations
-----------------------------------------------------
bench1           1.16 us         1.16 us       604100

 Performance counter stats for 'system wide':

 %  tma_backend_bound %  tma_retiring %  tma_frontend_bound %  tma_bad_speculation
                28.4           64.8                 6.3                    1.1
```

二级分析（toplev.py L2）：

```shell
$ toplev.py --core S0-C0 -l2 --no-desc taskset -c 0 ./lab
core BE               Backend_Bound              % Slots                       25.4
core RET              Retiring                   % Slots                       73.4
core BE/Core          Backend_Bound.Core_Bound   % Slots                       25.0
core RET              Retiring.Light_Operations   % Slots                       73.4  <==
```

---

## 优化分析

### 性能对比

| 指标 | 优化前 | 优化后 | 变化 |
|------|--------|--------|------|
| benchmark 耗时 | 24.3 μs | 1.16 μs | **20.9x 加速** |
| Core Bound | 74.9% | 25.0% | **-49.9%** |
| Retiring | 23.4% | 73.4% | **+50.0%** |
| Backend Bound | 74.9% | 25.4% | -49.5% |

### 为什么有效

1. **消除依赖链**：原始代码 `acc += acc < value` 每次迭代依赖上一次的 `acc`，形成严格串行链。拓宽为 `uint32_t` 后，循环内只剩 `acc += value`，编译器可以将多个加法并行执行（循环展开 + SIMD 向量化）。

2. **Retiring 从 23.4% → 73.4%**：优化前 CPU 大部分时间在等待依赖链完成（Core Bound 74.9%）。优化后 CPU 执行槽被有效指令填满（Light Operations 73.4%），说明向量化后的加法指令密集且高效。

3. **进位修正的正确性**：64K 个 max `uint16_t`(65535) 的总和 = 64×1024×65535 = 4,294,836,480 < 2^32 = 4,294,967,296，所以 `uint32_t` 不会溢出。循环结束后只需 2 次进位修正即可得到正确的 `uint16_t` 结果。

### 依赖链示意

```
优化前 (uint16_t acc，每次迭代有进位依赖)：

  iter 0: acc = 0 + blob[0]        → 检查进位 → acc'
  iter 1: acc' + blob[1]           → 检查进位 → acc''   ← 必须等 iter 0
  iter 2: acc'' + blob[2]          → 检查进位 → acc'''  ← 必须等 iter 1
  ...完全串行，64K 次迭代不可并行

优化后 (uint32_t acc，无进位检查)：

  iter 0: acc += blob[0]  ─┐
  iter 1: acc += blob[1]  ─┤ 可并行！编译器向量化
  iter 2: acc += blob[2]  ─┤
  iter 3: acc += blob[3]  ─┘
  ...4 个一组并行执行，共 16K 组

  最后：acc = low + high + carry  ← 一次性修正
```

### 为什么加速 20.9x 而非理论的更大值

1. **内存带宽**：128KB 数据需要从缓存加载，L1 48KB 可装 3/8，其余走 L2/L3。
2. **向量化宽度**：AVX2 一次处理 16 个 `uint16_t`（或 8 个 `uint32_t`），理论最大加速 ~8x（寄存器宽度），实际 20.9x 包含了循环展开和乱序执行的额外收益。
3. **进位修正开销**：循环外的 2 次进位修正可忽略不计。
