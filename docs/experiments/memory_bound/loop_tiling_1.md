# Loop Tiling

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

矩阵转置 `out[i][j] = in[j][i]`，2000×2000 的 `double` 矩阵（`vector<vector<double>>`）。

关键数据：每行 2000×8B = 16KB，整矩阵 ~32MB。L1 Data 48KB，L2 3MB。

```
内存布局（vector<vector<double>>，每行独立堆分配）：

  in[0]: [0][0] [0][1] ... [0][1999]   ← 16KB，地址 A
  in[1]: [1][0] [1][1] ... [1][1999]   ← 16KB，地址 B（不一定连续）
  ...
  in[1999]: ...                         ← 16KB，地址 Z
```

---

## 优化前

### 原始代码

```c++
bool solution(MatrixOfDoubles &in, MatrixOfDoubles &out) {
    int size = in.size();
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            out[i][j] = in[j][i];  // in[j][i] 列访问，stride = 16KB
        }
    }
    return out[0][size - 1];
}
```

**问题：** 内层 j 循环中 `in[j][i]` 是列优先访问——每次 j+1 跳到下一行的同一列（stride = 一行地址间隔 ≈ 16KB+），远超缓存行大小（64B），cache miss 频繁。

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
bench1           7.56 ms         7.56 ms          368
```

### Profile

一级分析（Top-Down）：

```shell
$ perf stat --topdown -a taskset -c 0 ./lab
-----------------------------------------------------
Benchmark           Time             CPU   Iterations
-----------------------------------------------------
bench1           7.86 ms         7.86 ms           85

 Performance counter stats for 'system wide':

 %  tma_backend_bound %  tma_retiring %  tma_frontend_bound %  tma_bad_speculation
                36.9            7.2                 6.0                    0.8
```

二级分析（toplev.py L2）：

```shell
$ toplev.py --core S0-C0 -l2 --no-desc taskset -c 0 ./lab
core BE               Backend_Bound               % Slots                       90.6
core BE/Mem           Backend_Bound.Memory_Bound   % Slots                       71.5  <==
core BE/Core          Backend_Bound.Core_Bound     % Slots                       19.1
```

**瓶颈分析：** Memory Bound 高达 71.5%。`in[j][i]` 列优先访问导致每读一个元素都可能触发 cache miss，CPU 执行单元大部分时间在等待内存数据返回。

---

## 优化后

### 优化后代码

引入 TILE_SIZE=16 的分块循环，将 2000×2000 矩阵划分为 16×16 的小块处理：

```c++
bool solution(MatrixOfDoubles &in, MatrixOfDoubles &out) {
    static constexpr int TILE_SIZE = 16;
    int size = in.size();
    for (int ii = 0; ii < size; ii += TILE_SIZE) {
        for (int jj = 0; jj < size; jj += TILE_SIZE) {
            for (int i = ii; i < std::min(ii + TILE_SIZE, size); i++) {
                for (int j = jj; j < std::min(jj + TILE_SIZE, size); j++) {
                    out[i][j] = in[j][i];
                }
            }
        }
    }
    return out[0][size - 1];
}
```

**改进点：** 将 2000×2000 的大矩阵划分为 16×16 的小块。在每个块内，`in[j][i]` 的行跨度从 2000 缩小到 16，多次访问可以复用同一批缓存行。

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
bench1           4.32 ms         4.32 ms          683
```

### Profile

一级分析（Top-Down）：

```shell
$ perf stat --topdown -a taskset -c 0 ./lab
-----------------------------------------------------
Benchmark           Time             CPU   Iterations
-----------------------------------------------------
bench1           4.34 ms         4.34 ms          168

 Performance counter stats for 'system wide':

 %  tma_backend_bound %  tma_retiring %  tma_frontend_bound %  tma_bad_speculation
                50.0           12.5                10.9                    1.8
```

二级分析（toplev.py L2）：

```shell
$ toplev.py --core S0-C0 -l2 --no-desc taskset -c 0 ./lab
core BE               Backend_Bound               % Slots                       80.4
core BE/Mem           Backend_Bound.Memory_Bound   % Slots                       67.1  <==
core BE/Core          Backend_Bound.Core_Bound     % Slots                       13.3
```

---

## 优化分析

### 性能对比

| 指标           | 优化前（无 tiling） | 优化后（tiling + vector-of-vector） | 平坦数组（tiling + flat） | 变化（前→平坦）   |
| -------------- | ------------------- | ----------------------------------- | ------------------------ | ---------------- |
| benchmark 耗时 | 7.56 ms             | 4.32 ms                             | 2.85 ms                  | **2.65x 加速**   |
| Memory Bound   | 71.5%               | 67.1%                               | —                        | —                |
| Core Bound     | 19.1%               | 13.3%                               | —                        | —                |
| Retiring       | 7.2%                | 12.5%                               | —                        | —                |

### 为什么有效

1. **工作集缩小**：未分块时，内层 j 循环需要访问 `in[0..1999][i]` 共 2000 行的不同行，工作集 = 2000×16KB = 32MB，远超 L2（3MB）。分块后，每个 16×16 块只需访问 16 行，工作集 = 16×16×8B = 2KB，完全在 L1 内。

2. **缓存行复用**：分块后 `in[j][i]` 在块内 j 方向的跨度缩小到 16 行。当 j 从 0 增长到 15 时，`in[j]` 的 16 行数据可以留在 L1/L2 中被复用。

3. **预取友好**：块内连续的内存访问模式让硬件预取器能有效工作。

### 分块示意

```
原始 (i-j 循环)：
  i=0: in[0][0], in[1][0], in[2][0], ... in[1999][0]  ← 跳 2000 行
  i=1: in[0][1], in[1][1], in[2][1], ... in[1999][1]
  ...每行访问 2000 个不同行的同一列

分块 (TILE_SIZE=16)：
  块(0,0): i=0..15, j=0..15  → 只访问 in[0..15] 的 16 行 ← L1 可容纳
  块(0,1): i=0..15, j=16..31 → 同样只访问 16 行
  ...
  共 (2000/16)² = 15625 个块
```

### 局限性

本场景使用 `vector<vector<double>>`，每行是独立堆分配，行间不连续。这限制了 tiling 的效果——即使块内行数减少，行间的地址跳跃仍然存在。若使用连续内存布局（如一维数组 `double* data`），效果会更显著。

---

## 连续内存布局对比

为了验证上述局限性，新增基于一维数组 `vector<double>`（N×N 连续内存）的实现，与 `vector<vector<double>>` 进行对比。

### 平坦数组实现

```c++
bool solution_flat(FlatMatrix& in, FlatMatrix& out, int N) {
    static constexpr int TILE_SIZE = 16;

    for (int ii = 0; ii < N; ii += TILE_SIZE) {
        for (int jj = 0; jj < N; jj += TILE_SIZE) {
            for (int i = ii; i < std::min(ii + TILE_SIZE, N); i++) {
                for (int j = jj; j < std::min(jj + TILE_SIZE, N); j++) {
                    out[i * N + j] = in[j * N + i];  // 连续内存，行间无跳跃
                }
            }
        }
    }
    return out[0 * N + (N - 1)];
}
```

**关键区别：** `vector<double>` 将整个 N×N 矩阵存储在一块连续内存中，元素 `(i,j)` 的地址为 `base + (i*N + j) * 8B`。行与行之间在地址上紧密相邻，不存在堆分配碎片。

### 验证正确性

```shell
$ ./validate
Validation Successful (vector-of-vector)
Validation Successful (flat array)
```

### 运行 benchmark

```shell
$ cmake --build . --target benchmarkLab
-----------------------------------------------------
Benchmark           Time             CPU   Iterations
-----------------------------------------------------
bench1           3.52 ms         3.52 ms          195
bench_flat       2.85 ms         2.85 ms          249
```

### 为什么平坦数组更快

1. **行间地址连续**：`vector<vector<double>>` 每行独立 `new` 分配，行间地址不连续，即使 tiling 缩小了行跨度，块内 16 行仍可能散布在堆的不同位置。平坦数组中，相邻行在内存中紧挨着，cache line 预取完全命中。

2. **TLB 压力更低**：非连续布局下，访问 16 行可能跨越多个 4KB 页（每行 16KB，行间有堆元数据开销），增加 TLB miss。平坦数组是单块连续内存，页表访问模式规则，TLB 命中率更高。

3. **硬件预取更有效**：连续内存的访问模式（stride = 8B）对硬件预取器友好；非连续布局的行间跳跃（stride = 16KB + 堆间隔）超出了预取器的检测范围。

### 内存布局对比

```
vector<vector<double>>（非连续）：
  in[0]: [0..1999] @ 0x7f0010     ← 堆分配 A
  in[1]: [0..1999] @ 0x7f0020     ← 堆分配 B（地址不连续）
  in[2]: [0..1999] @ 0x7f0038     ← 堆分配 C（间隔不规则）
  ...
  Tiling 后块内访问 16 行，但行间仍有地址跳跃

vector<double>（连续）：
  data: [0..3999999] @ 0x7f0010   ← 单块 32MB 连续内存
  行 i 起始地址 = base + i * 16000
  行 i+1 起始地址 = base + (i+1) * 16000  ← 紧邻，无间隔
  Tiling 后块内 16 行完全连续，cache line 利用率 100%
```
