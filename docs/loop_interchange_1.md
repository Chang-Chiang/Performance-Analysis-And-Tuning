# Loop Interchange

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

## 优化前

### 原始代码

矩阵乘法 `C = A × B`，400×400 的 `float` 矩阵，循环顺序为 **i-j-k**：

```c++
void multiply(Matrix &result, const Matrix &a, const Matrix &b) {
    zero(result);

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            for (int k = 0; k < N; k++) {
                result[i][j] += a[i][k] * b[k][j];
            }
        }
    }
}
```

**问题：** 内层循环沿 `k` 迭代时，`b[k][j]` 的访问模式是 **列优先**（stride = N = 400），每次访问都跳过一整行（1600 字节），无法利用缓存行预取，导致大量 cache miss。

### 验证正确性

```shell
$ ./validate
Validation Successful
```

### 运行 benchmark

benchmark 计算 `A^2021`（快速幂，多次调用 `multiply`）：

```shell
$ cmake --build . --target benchmarkLab
---------------------------------------------------------------
Benchmark                     Time             CPU   Iterations
---------------------------------------------------------------
bench1/iterations:10  398306166 ns    398250028 ns           10
```

### Profile

一级分析（Top-Down）：

```shell
$ perf stat --topdown -a taskset -c 0 ./lab
---------------------------------------------------------------
Benchmark                     Time             CPU   Iterations
---------------------------------------------------------------
bench1/iterations:10  401354462 ns    401265491 ns           10

 Performance counter stats for 'system wide':

 %  tma_backend_bound %  tma_retiring %  tma_frontend_bound %  tma_bad_speculation
                32.9           52.4                 3.3                    1.7
```

二级分析（toplev.py L2）：

```shell
$ toplev.py --core S0-C0 -l2 --no-desc taskset -c 0 ./lab
# 5.1-full on Intel(R) Core(TM) Ultra 7 270K Plus [arl]
core BE               Backend_Bound             % Slots                       43.1
core BE/Core          Backend_Bound.Core_Bound  % Slots                       41.2  <==
```

**瓶颈分析：** Core Bound 高达 41.2%，说明 CPU 执行单元被阻塞。根因是 `b[k][j]` 列优先访问导致 L1/L2 cache miss 频繁，流水线因等待数据而停顿。

> **为什么是 Core Bound 而不是 Memory Bound？**
>
> 在 TMA 框架中，Backend Bound 分为两个子类：
>
> | 类别 | 含义 | 典型原因 |
> |------|------|---------|
> | **Core Bound** | 流水线因**核内资源/延迟**而停顿 | L1/L2 cache miss、数据依赖、长延迟指令、执行端口争用 |
> | **Memory Bound** | 流水线因**核外内存子系统**而停顿 | L3 cache miss、DRAM 访问延迟 |
>
> 关别在于**延迟的量级**：L1 miss → L2 hit 约 12 个 cycle，属于核内停顿，归为 Core Bound；L3 miss → DRAM 约 100+ 个 cycle，属于核外停顿，归为 Memory Bound。本例中 `b[k][j]` 列优先访问导致的是 L1/L2 cache miss，停顿发生在核内缓存层级，因此被归类为 Core Bound。
>
> 可用更深层 TMA 分析验证：`toplev.py --core S0-C0 -l3 --no-desc taskset -c 0 ./lab`，若 L3 级别显示 `Core_Bound.L1_Bound` 或 `Core_Bound.L2_Bound` 较高，即确认瓶颈在核内缓存层面。

---

## 优化后

### 优化后代码

交换 j 和 k 的循环顺序，变为 **i-k-j**：

```c++
void multiply(Matrix &result, const Matrix &a, const Matrix &b) {
    zero(result);

    for (int i = 0; i < N; i++) {
        for (int k = 0; k < N; k++) {
            for (int j = 0; j < N; j++) {
                result[i][j] += a[i][k] * b[k][j];
            }
        }
    }
}
```

**改进点：** 内层循环沿 `j` 迭代时，`result[i][j]` 和 `b[k][j]` 都是 **行优先** 连续访问（stride = 1），充分利用缓存行预取，cache miss 大幅减少。

### 验证正确性

```shell
$ ./validate
Validation Successful
```

### 运行 benchmark

```shell
$ cmake --build . --target benchmarkLab
---------------------------------------------------------------
Benchmark                     Time             CPU   Iterations
---------------------------------------------------------------
bench1/iterations:10   57407502 ns     57405073 ns           10
```

### Profile

一级分析（Top-Down）：

```shell
$ perf stat --topdown -a taskset -c 0 ./lab
---------------------------------------------------------------
Benchmark                     Time             CPU   Iterations
---------------------------------------------------------------
bench1/iterations:10   59460681 ns     59441782 ns           10

 Performance counter stats for 'system wide':

 %  tma_backend_bound %  tma_retiring %  tma_frontend_bound %  tma_bad_speculation
                33.4           33.8                 1.8                    0.6
```

二级分析（toplev.py L2）：

```shell
$ toplev.py --core S0-C0 -l2 --no-desc taskset -c 0 ./lab
# 5.1-full on Intel(R) Core(TM) Ultra 7 270K Plus [arl]
core BE               Backend_Bound               % Slots                       64.5
core BE/Mem           Backend_Bound.Memory_Bound   % Slots                       22.3
core BE/Core          Backend_Bound.Core_Bound     % Slots                       42.2  <==
```

---

## 优化分析

### 性能对比

| 指标            | 优化前 (i-j-k) | 优化后 (i-k-j) | 提升          |
| --------------- | -------------- | -------------- | ------------- |
| benchmark 耗时  | 398 ms         | 57.4 ms        | **6.9x 加速** |
| Backend Bound   | 43.1%          | 64.5%          | —             |
| Core Bound      | 41.2%          | 42.2%          | 持平          |
| Memory Bound    | —              | 22.3%          | —             |
| Frontend Bound  | 3.3%           | 1.8%           | -1.5%         |
| Bad Speculation | 1.7%           | 0.6%           | -1.1%         |

> **为什么 Backend Bound 反而升高了？** 优化前程序大部分时间在等待内存（cache miss），但这些等待被计入了 Retiring（52.4%）而非 Backend Bound，因为 CPU 流水线并未真正"繁忙"。优化后 cache 命中率提升，CPU 能更高效地发射指令，Backend Bound 的绝对值虽然百分比升高，但总耗时从 398ms 降至 57ms，实际后端等待时间大幅缩短。

### 为什么有效

1. **缓存行利用率**：`float` 矩阵每行 400×4B = 1600B = 25 个缓存行。i-j-k 顺序下内层循环每次访问 `b[k][j]` 跨越整行，stride = 1600B；i-k-j 顺序下 `b[k][j]` 连续访问，stride = 4B。

2. **硬件预取**：连续内存访问模式让 CPU 硬件预取器能准确预测下一次访问地址，提前将数据拉入 L1 缓存。

3. **SIMD 友好**：连续访问使编译器能自动向量化内层循环（`j` 循环），用 AVX/AVX2 一次处理 8 个 `float`。

4. **矩阵幂的放大效应**：`power(a, 2021)` 需要多次调用 `multiply`，单次乘法的优化被指数级放大。

### 访问模式图示

```
矩阵 B (400×400) 的内存布局（行优先）：

  ┌─────────────────────────────────────────┐
  │ row 0: [0][0] [0][1] [0][2] ... [0][399]│  ← 连续 1600B
  │ row 1: [1][0] [1][1] [1][2] ... [1][399]│
  │ ...                                     │
  │ row 399: ...                            │
  └─────────────────────────────────────────┘

i-j-k (原始): 内层 k 循环访问 b[k][j]
  → b[0][j], b[1][j], b[2][j], ...  stride = 1600B ✗ 跳行

i-k-j (优化): 内层 j 循环访问 b[k][j]
  → b[k][0], b[k][1], b[k][2], ...  stride = 4B    ✓ 连续
```
