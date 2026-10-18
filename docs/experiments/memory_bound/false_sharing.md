# False Sharing

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

OpenMP 多线程累加：每个线程有自己的 `std::atomic<uint32_t>` 计数器，最后求和。1M 个 `uint32_t` 数据，线程数从 1 到 24。

```
False Sharing 发生过程：

  线程 0 的 accumulator[0]  ┐
  线程 1 的 accumulator[1]  │ 同一个 64B 缓存行！
  线程 2 的 accumulator[2]  │
  线程 3 的 accumulator[3]  ┘

  线程 0 写 accumulator[0] → 缓存行状态变为 Exclusive
  线程 1 要写 accumulator[1] → 发现缓存行被其他核心持有
  → 触发 MESI 协议的缓存行无效化 → 线程 1 等待
  → 所有线程串行化，多核并行失效
```

---

## 优化前

### 原始代码

```c++
struct Accumulator {
    std::atomic<uint32_t> value = 0;
};
std::vector<Accumulator> accumulators(thread_count);
```

`sizeof(Accumulator) = 4B`，多个线程的计数器紧挨着放在同一缓存行中，触发 false sharing。

### 验证正确性

```shell
$ ./validate
Validation Successful
```

### 运行 benchmark

benchmark 从 1 线程跑到 24 线程：

```shell
$ cmake --build . --target benchmarkLab
-----------------------------------------------------------
Benchmark                 Time             CPU   Iterations
-----------------------------------------------------------
bench1/real_time        324 ms          163 ms            9
```

> real_time=324ms 远大于 CPU=163ms，说明大量时间花在等待（缓存一致性协议导致的线程阻塞）。

### Profile

一级分析（Top-Down）：

```shell
$ perf stat --topdown -a taskset -c 0 ./lab
-----------------------------------------------------------
Benchmark                 Time             CPU   Iterations
-----------------------------------------------------------
bench1/real_time       96.7 ms         15.3 ms            7

 Performance counter stats for 'system wide':

 %  tma_backend_bound %  tma_retiring %  tma_frontend_bound %  tma_bad_speculation
                44.6           17.8                11.8                    1.0
```

二级分析（toplev.py L2）：

```shell
$ toplev.py --core S0-C0 -l2 --no-desc taskset -c 0 ./lab
core BE               Backend_Bound               % Slots                       79.6
core BE/Mem           Backend_Bound.Memory_Bound   % Slots                       28.2
core BE/Core          Backend_Bound.Core_Bound     % Slots                       51.4  <==
```

**瓶颈分析：** Core Bound 51.4%（非 Memory Bound），说明 CPU 执行单元被阻塞——不是在等内存数据，而是在等缓存一致性协议完成。其他核心修改了同一缓存行，本核心必须等待缓存行重新获得独占权才能写入。

perf c2c 检测：

```shell
$ perf c2c record ./lab
$ perf c2c report --stdio
  Load Local HITM             :        153    ← 缓存行被其他核心修改后读取
  Locked Load/Store Operations:      34024    ← 原子操作导致的锁定访问
  Total Shared Cache Lines    :        141    ← 共享缓存行数量
```

---

## 优化后

### 优化后代码

用 `alignas(64)` 将每个 Accumulator 对齐到缓存行边界，确保不同线程的计数器在不同缓存行上：

```c++
#define CACHELINE_ALIGN alignas(64)
struct CACHELINE_ALIGN Accumulator {
    std::atomic<uint32_t> value = 0;
};
std::vector<Accumulator> accumulators(thread_count);
```

`sizeof(Accumulator)` 从 4B 膨胀到 64B，但每个线程独占一个缓存行，消除了 false sharing。

### 验证正确性

```shell
$ ./validate
Validation Successful
```

### 运行 benchmark

```shell
$ cmake --build . --target benchmarkLab
-----------------------------------------------------------
Benchmark                 Time             CPU   Iterations
-----------------------------------------------------------
bench1/real_time       20.5 ms         15.6 ms          138
```

### Profile

二级分析（toplev.py L2）：

```shell
$ toplev.py --core S0-C0 -l2 --no-desc taskset -c 0 ./lab
core BE               Backend_Bound               % Slots                       80.4
core BE/Mem           Backend_Bound.Memory_Bound   % Slots                       31.0
core BE/Core          Backend_Bound.Core_Bound     % Slots                       49.4  <==
```

---

## 优化分析

### 性能对比

| 指标         | 优化前 | 优化后  | 变化           |
| ------------ | ------ | ------- | -------------- |
| real_time    | 324 ms | 20.5 ms | **15.8x 加速** |
| CPU time     | 163 ms | 15.6 ms | 持平           |
| Core Bound   | 51.4%  | 49.4%   | -2.0%          |
| Memory Bound | 28.2%  | 31.0%   | +2.8%          |

### 为什么有效

1. **real_time vs CPU time**：优化前 real_time(324ms) >> CPU time(163ms)，差值 161ms 是线程等待缓存一致性协议的时间。优化后 real_time(20.5ms) ≈ CPU time(15.6ms)，等待时间几乎消除。

2. **MESI 协议开销**：false sharing 时，每次写入都触发缓存行在核心间"弹跳"（Invalidate → Shared → Exclusive 状态转换），每次转换 ~40-80ns。4 个线程 × 1M 次迭代 = 数千万次无效化。

3. **内存换速度**：`alignas(64)` 使内存用量从 `thread_count × 4B` 膨胀到 `thread_count × 64B`（24 线程 = 1.5KB），代价可忽略不计。

### 缓存行布局对比

```
优化前 (sizeof=4B，多个线程挤在同一缓存行)：

  缓存行 0 (64B):
  ┌──────────────┬──────────────┬─────┬─────────┐
  │ acc[0].value │ acc[1].value │ ... │ acc[15] │
  ├──────────────┼──────────────┼─────┼─────────┤
  │    4B        │    4B        │     │   4B    │
  └──────────────┴──────────────┴─────┴─────────┘
  线程 0 写 acc[0] → 线程 1 写 acc[1] → 缓存行弹跳！

优化后 (alignas(64)，每个线程独占缓存行)：

  缓存行 0: ┌──────────────┬─────────────────────┐
           │ acc[0].value │ padding (60B)       │ ← 线程 0 独占
           └──────────────┴─────────────────────┘
  缓存行 1: ┌──────────────┬─────────────────────┐
           │ acc[1].value │ padding (60B)       │ ← 线程 1 独占
           └──────────────┴─────────────────────┘
  ...
  各线程互不干扰，无缓存行弹跳
```

### 何时考虑 False Sharing

| 信号                           | 说明                              |
| ------------------------------ | --------------------------------- |
| real_time >> CPU time          | 线程在等待而非计算                |
| Core Bound 高，Memory Bound 低 | 不是缺内存带宽，而是缓存一致性    |
| 多线程写相邻小对象             | `atomic`、计数器、per-thread 状态 |
| perf c2c 显示 HITM 高          | 缓存行在核心间频繁迁移            |
