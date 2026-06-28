# Vectorization (Sequence Alignment)

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

Smith-Waterman 风格的仿射间隙惩罚序列比对算法，处理 16 对长度为 200 的 DNA 序列（4 字母表）。

核心递推公式（每个单元格）：

```
score[row][col] = max(
    diagonal + match/mismatch,   // 对角线方向
    vertical_gap,                // 垂直间隙
    horizontal_gap               // 水平间隙
)
```

**问题：** 16 对序列依次处理（标量方式），每次只算 1 个 `int16_t`，无法利用 SIMD 宽度（AVX2 可一次处理 16 个 `int16_t`）。

---

## 优化前

### 原始代码

逐序列标量计算，外层循环遍历 16 对序列，内层循环做单个序列的比对：

```c++
result_t compute_alignment(...) {
    result_t result{};

    for (size_t sequence_idx = 0; sequence_idx < sequences1.size(); ++sequence_idx) {
        using score_t = int16_t;           // 标量：每次算 1 个 int16_t
        using column_t = std::array<score_t, sequence_size_v + 1>;

        // ... 初始化 ...

        for (unsigned col = 1; col <= sequence2.size(); ++col) {
            for (unsigned row = 1; row <= sequence1.size(); ++row) {
                score_t best_cell_score =
                    last_diagonal_score +
                    (sequence1[row-1] == sequence2[col-1] ? match : mismatch);
                best_cell_score = std::max(best_cell_score, last_vertical_gap);
                best_cell_score = std::max(best_cell_score, horizontal_gap_column[row]);
                // ... 更新 gap 值 ...
            }
        }

        result[sequence_idx] = score_column.back();
    }
    return result;
}
```

**编译器无法自动向量化的原因：** 内层 row 循环存在循环依赖——`last_diagonal_score` 依赖上一次迭代的 `score_column[row]`，`last_vertical_gap` 同理。编译器无法证明这些依赖可以安全地向量化。

### 验证正确性

```shell
$ ./validate
Validation Successful
```

### 运行 benchmark

```shell
$ cmake --build . --target benchmarkLab
------------------------------------------------------------------
Benchmark                        Time             CPU   Iterations
------------------------------------------------------------------
bench_compute_alignment     830630 ns       830434 ns          842
```

### Profile

一级分析（Top-Down）：

```shell
$ perf stat --topdown -a taskset -c 0 ./lab
------------------------------------------------------------------
Benchmark                        Time             CPU   Iterations
------------------------------------------------------------------
bench_compute_alignment     830630 ns       830434 ns          842

 Performance counter stats for 'system wide':

 %  tma_backend_bound %  tma_retiring %  tma_frontend_bound %  tma_bad_speculation
                28.9           38.5                 6.7                    1.4
```

二级分析（toplev.py L2）：

```shell
$ toplev.py --core S0-C0 -l2 --no-desc taskset -c 0 ./lab
core BE               Backend_Bound             % Slots                       55.3
core BE/Core          Backend_Bound.Core_Bound   % Slots                       55.3  <==
```

**瓶颈分析：** Core Bound 55.3%，说明 CPU 执行单元被阻塞。内层 row 循环存在 `last_diagonal_score` 和 `last_vertical_gap` 的依赖链，每次迭代依赖上一次的结果，CPU 流水线无法充分利用。

---

## 优化后

### 优化后思路

**在数据中找并行性，而非在代码中。** 16 对序列之间完全独立，可以同时处理。将数据从 AoS（Array of Structures）转为 SoA（Structure of Arrays），用 `std::array<int16_t, 16>` 作为"向量化的 score 类型"，一次处理 16 对序列的同一位置。

### 优化后代码

```c++
// 向量化类型：16 个 int16_t 打包在一起
using simd_score_t = std::array<int16_t, sequence_count_v>;  // 16 个 score 并行
using simd_sequence_t = std::array<simd_score_t, sequence_size_v>;

// 转置：AoS → SoA
simd_sequence_t transpose(std::vector<sequence_t> const &sequences) {
    simd_sequence_t simd_sequence{};
    for (size_t i = 0; i < simd_sequence.size(); ++i)
        for (size_t j = 0; j < sequences.size(); ++j)
            simd_sequence[i][j] = sequences[j][i];
    return simd_sequence;
}

result_t compute_alignment(...) {
    result_t result{};

    // 1. 转置数据：16 个独立序列 → 16 通道并行
    auto trSeq1 = transpose(sequences1);
    auto trSeq2 = transpose(sequences2);

    using score_t = simd_score_t;  // 标量 int16_t → 向量 array<int16_t,16>
    using column_t = std::array<score_t, sequence_size_v + 1>;

    // 2. 所有标量值变为向量
    score_t gap_open{};
    gap_open.fill(-11);
    // ... match, mismatch, gap_extension 同理 ...

    // 3. 主递推：一次处理 16 对序列的同一位置
    for (unsigned col = 1; col <= trSeq2.size(); ++col) {
        for (unsigned row = 1; row <= trSeq1.size(); ++row) {
            score_t best_cell_score = last_diagonal_score;
            for (size_t k = 0; k < sequence_count_v; ++k) {
                // 16 通道并行计算 match/mismatch
                best_cell_score[k] += (trSeq1[row-1][k] == trSeq2[col-1][k]
                                       ? match[k] : mismatch[k]);
            }
            for (size_t k = 0; k < sequence_count_v; ++k) {
                // 16 通道并行计算 max
                best_cell_score[k] = std::max(best_cell_score[k], last_vertical_gap[k]);
                best_cell_score[k] = std::max(best_cell_score[k], horizontal_gap_column[row][k]);
                // ... 更新 gap 值 ...
            }
        }
    }

    for (size_t k = 0; k < sequence_count_v; ++k)
        result[k] = score_column.back()[k];

    return result;
}
```

**关键改进：** `score_t` 从 `int16_t`（1 个值）变为 `std::array<int16_t, 16>`（16 个值）。编译器看到 `for (k = 0; k < 16; ++k)` 循环对连续数组操作，会自动向量化为 AVX2 指令（`_mm256_*` 系列）。

### 验证正确性

```shell
$ ./validate
Validation Successful
```

### 运行 benchmark

```shell
$ cmake --build . --target benchmarkLab
------------------------------------------------------------------
Benchmark                        Time             CPU   Iterations
------------------------------------------------------------------
bench_compute_alignment     396270 ns       396253 ns         1763
```

### Profile

一级分析（Top-Down）：

```shell
$ perf stat --topdown -a taskset -c 0 ./lab
------------------------------------------------------------------
Benchmark                        Time             CPU   Iterations
------------------------------------------------------------------
bench_compute_alignment     396270 ns       396253 ns         1763

 Performance counter stats for 'system wide':

 %  tma_backend_bound %  tma_retiring %  tma_frontend_bound %  tma_bad_speculation
                43.5           23.8                 4.9                    0.5
```

二级分析（toplev.py L2）：

```shell
$ toplev.py --core S0-C0 -l2 --no-desc taskset -c 0 ./lab
core BE               Backend_Bound             % Slots                       73.7
core BE/Core          Backend_Bound.Core_Bound   % Slots                       58.8  <==
```

---

## 优化分析

### 性能对比

| 指标 | 优化前（标量） | 优化后（SIMD） | 变化 |
|------|---------------|---------------|------|
| benchmark 耗时 | 830.6 μs | 396.3 μs | **2.1x 加速** |
| Backend Bound | 55.3% | 73.7% | — |
| Core Bound | 55.3% | 58.8% | 持平 |
| Retiring | 38.5% | 23.8% | -14.7% |
| 每次处理 | 1 个 int16_t | 16 个 int16_t | 16x 数据并行 |

> **为什么 Retiring 从 38.5% 降到 23.8%？** SIMD 版本总耗时更短（396μs vs 831μs），CPU 在更短时间内完成了相同工作。Retiring 百分比降低是因为 Backend Bound 百分比升高（73.7%），后端执行槽占比更大——这反映了 SIMD 指令更充分利用了执行单元。

### 为什么有效

1. **数据级并行**：16 对序列完全独立，天然适合 SIMD。将 AoS 转为 SoA 后，`std::array<int16_t, 16>` 在内存中连续排列，编译器可自动向量化为 AVX2 指令。

2. **编译器友好的循环结构**：`for (k = 0; k < 16; ++k)` 是固定次数、无依赖的连续内存访问循环，编译器可以放心向量化。而原始的 row 循环存在 `last_diagonal_score` 依赖链，无法向量化。

3. **缓存友好**：转置后 `trSeq1[row]` 和 `trSeq2[col]` 都是 16B 连续数据（16 × 1B），一次缓存行加载即可获取所有通道的数据。

### 数据布局变换

```
原始 AoS (16 个独立序列)：
  sequences[0] = [A,C,G,T,...]  ← 200B
  sequences[1] = [G,T,A,C,...]  ← 200B
  ...
  sequences[15] = [T,A,G,C,...] ← 200B

转置后 SoA (16 通道并行)：
  trSeq[0]   = [seq0[0], seq1[0], ..., seq15[0]]  ← 16B，16 通道第 0 个元素
  trSeq[1]   = [seq0[1], seq1[1], ..., seq15[1]]  ← 16B，16 通道第 1 个元素
  ...
  trSeq[199] = [seq0[199], seq1[199], ..., seq15[199]] ← 16B

  每个 trSeq[i] 正好是一个 AVX2 寄存器宽度（256b = 32B 可装 16 个 int16_t）
```

### 为什么加速只有 2.1x 而非 16x

1. **转置开销**：AoS → SoA 的转置需要额外的内存读写。
2. **循环结构**：优化后的代码用显式 `for (k)` 循环而非编译器内在函数，编译器可能未完全向量化。
3. **数据依赖**：row 循环内仍有 `last_diagonal_score` 的依赖链（跨行），限制了行维度的并行。
4. **内存带宽**：200×200 的 score 矩阵访问模式仍有改进空间。
