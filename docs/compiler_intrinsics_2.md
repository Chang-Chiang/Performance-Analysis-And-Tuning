# Compiler Intrinsics (Longest Line)

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

在文本文件中查找最长行的长度。输入是两个文件：`LoopVectorize.cpp`（LLVM 源码，长行多）和 `MarkTwain-TomSawyer.txt`（小说，行较长）。

```
算法：逐字符扫描
  遇到 '\n' → 重置当前行长度
  否则 → 当前行长度 +1
  维护全局最大值
```

**问题：** 逐字符处理，每次迭代都要比较 `s == '\n'`，分支预测器难以准确预测（换行符出现位置不规律），导致大量分支预测失败。

---

## 优化前

### 原始代码

```c++
unsigned solution(const std::string &inputContents) {
  unsigned longestLine = 0;
  unsigned curLineLength = 0;

  for (auto s : inputContents) {
    curLineLength = (s == '\n') ? 0 : curLineLength + 1;
    longestLine = std::max(curLineLength, longestLine);
  }

  return longestLine;
}
```

### 验证正确性

```shell
$ ./validate ../inputs/
Validation Successful
```

### 运行 benchmark

```shell
$ cmake --build . --target benchmarkLab
-----------------------------------------------------
Benchmark           Time             CPU   Iterations
-----------------------------------------------------
bench1            180 us          180 us        15446
```

### Profile

一级分析（Top-Down）：

```shell
$ perf stat --topdown -a taskset -c 0 ./lab ../inputs/
-----------------------------------------------------
Benchmark           Time             CPU   Iterations
-----------------------------------------------------
bench1            180 us          180 us         3896

 Performance counter stats for 'system wide':

 %  tma_backend_bound %  tma_retiring %  tma_frontend_bound %  tma_bad_speculation
                24.0           37.1                16.3                   14.4
```

二级分析（toplev.py L2）：

```shell
$ toplev.py --core S0-C0 -l2 --no-desc taskset -c 0 ./lab ../inputs/
core BAD              Bad_Speculation                     % Slots                       16.1
core BE               Backend_Bound                       % Slots                       29.5
core BAD              Bad_Speculation.Branch_Mispredicts   % Slots                       16.1
core BE/Core          Backend_Bound.Core_Bound            % Slots                       29.5  <==
```

**瓶颈分析：** Bad Speculation 16.1%（全部来自 Branch Mispredicts）+ Core Bound 29.5%。逐字符循环中 `s == '\n'` 分支和 `std::max` 分支都难以预测，导致流水线频繁冲刷。

---

## 优化后

### 优化后代码

用 AVX2 intrinsics 一次处理 32 字节，用 SIMD 比较指令批量查找 `'\n'`，消除逐字符分支：

```c++
#include <immintrin.h>  // AVX2

uint32_t solution(const std::string &inputContents) {
    uint32_t longestLine = 0;
    uint32_t curLineLength = 0;
    uint32_t pos = 0;
    auto strLength = inputContents.size();

    if (strLength >= 32) {
        const __m256i eol = _mm256_set1_epi8('\n');
        auto *buffer = inputContents.data();
        uint32_t curLineBegin = 0;

        for (; pos + 32 < strLength; pos += 32) {
            // 1. 加载 32 字节，与 '\n' 比较
            __m256i vect = _mm256_loadu_si256((const __m256i *)buffer);
            __m256i vectMask = _mm256_cmpeq_epi8(vect, eol);
            uint32_t mask = _mm256_movemask_epi8(vectMask);

            // 2. 逐个处理 mask 中的 '\n' 位置
            while (mask) {
                int maskPos = __builtin_ctz(mask);  // 找最低位的 1

                uint32_t curLen = (pos - curLineBegin) + maskPos;
                if (pos < curLineBegin)
                    curLen = maskPos;

                curLineBegin += curLen + 1;
                longestLine = std::max(curLen, longestLine);

                ++maskPos;
                if (maskPos > 31) break;
                else mask >>= maskPos;
            }
            buffer += 32;
        }
        curLineLength = pos - curLineBegin;
    }

    // 剩余字符：标量循环
    for (; pos < strLength; pos++) {
        curLineLength = (inputContents[pos] == '\n') ? 0 : curLineLength + 1;
        longestLine = std::max(curLineLength, longestLine);
    }

    return longestLine;
}
```

**关键 intrinsics 说明：**

| Intrinsic | 作用 |
|-----------|------|
| `_mm256_set1_epi8('\n')` | 广播 `'\n'` 到 32 字节向量 |
| `_mm256_loadu_si256` | 加载 32 字节（未对齐） |
| `_mm256_cmpeq_epi8` | 32 字节逐字节比较，相等则全 1 |
| `_mm256_movemask_epi8` | 将 32 字节比较结果转为 32 位 mask |
| `__builtin_ctz` | 找 mask 中最低位 1 的位置（trailing zeros） |

### 验证正确性

```shell
$ ./validate ../inputs/
Validation Successful
```

### 运行 benchmark

```shell
$ cmake --build . --target benchmarkLab
-----------------------------------------------------
Benchmark           Time             CPU   Iterations
-----------------------------------------------------
bench1           12.0 us         11.9 us       233717
```

### Profile

一级分析（Top-Down）：

```shell
$ perf stat --topdown -a taskset -c 0 ./lab ../inputs/
-----------------------------------------------------
Benchmark           Time             CPU   Iterations
-----------------------------------------------------
bench1           11.9 us         11.9 us        53118

 Performance counter stats for 'system wide':

 %  tma_backend_bound %  tma_retiring %  tma_frontend_bound %  tma_bad_speculation
                34.1           48.7                 8.2                    4.2
```

二级分析（toplev.py L2）：

```shell
$ toplev.py --core S0-C0 -l2 --no-desc taskset -c 0 ./lab ../inputs/
core BE               Backend_Bound             % Slots                       39.2
core BE/Core          Backend_Bound.Core_Bound   % Slots                       38.4  <==
```

---

## 优化分析

### 性能对比

| 指标 | 优化前（标量） | 优化后（AVX2） | 变化 |
|------|---------------|---------------|------|
| benchmark 耗时 | 180 μs | 11.9 μs | **15.1x 加速** |
| Bad Speculation | 16.1% | 4.2% | **-11.9%** |
| Core Bound | 29.5% | 38.4% | +8.9% |
| Retiring | 37.1% | 48.7% | +11.6% |
| Frontend Bound | 16.3% | 8.2% | -8.1% |

### 为什么有效

1. **批量比较消除分支**：`_mm256_cmpeq_epi8` 一次比较 32 个字节与 `'\n'`，结果是一个 32 位 mask。只有当 mask 非零时才进入 `while` 循环处理换行符位置——大部分迭代中没有换行符，`while` 循环不执行，零分支。

2. **Bad Speculation 大幅下降（16.1% → 4.2%）**：原始版本每次迭代都有 `s == '\n'` 分支，换行符出现位置不规律导致预测失败。SIMD 版本将分支转换为位运算（mask），预测失败仅发生在 `while (mask)` 循环中。

3. **Frontend Bound 下降（16.3% → 8.2%）**：SIMD 版本循环体更紧凑（32 字节/迭代 vs 1 字节/迭代），循环次数减少 32 倍，前端取指压力降低。

### SIMD 查找示意

```
输入: "Hello\nWorld\nFoo\nBar........\n"
       ↓ AVX2 一次加载 32 字节
       ↓ _mm256_cmpeq_epi8(vect, '\n')
mask:  00000100 00001000 00000010 00000000 00000000 00000001 ...
       ↑        ↑        ↑                                  ↑
       pos 5    pos 11   pos 15                         pos 31

用 __builtin_ctz 逐个提取换行符位置:
  maskPos=5  → 行长度 = 5
  maskPos=11 → 行长度 = 5
  maskPos=15 → 行长度 = 3
  ...
```

### 为什么加速 15.1x

| 因素 | 贡献 |
|------|------|
| 32 字节/迭代 vs 1 字节/迭代 | ~32x 循环次数减少 |
| 消除分支预测失败 | ~10-20% 额外收益 |
| while 循环开销（处理换行符） | 部分抵消 |
| **净加速** | **15.1x** |
