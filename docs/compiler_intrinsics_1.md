# Compiler Intrinsics (Image Smoothing)

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

1D 图像平滑（滑动窗口求和），对 40000 个 `uint8_t` 元素计算半径为 13 的窗口和（窗口大小 27）。

```
算法：滑动窗口求和
  output[pos] = Σ input[pos-radius .. pos+radius]

优化点：主循环用差分递推
  currentSum -= input[pos - radius - 1]   // 移除左端
  currentSum += input[pos + radius]       // 添加右端
  output[pos] = currentSum
```

**问题：** 每次迭代只有 2 次整数运算（add + subtract），但有循环依赖（`currentSum` 跨迭代依赖），编译器无法自动向量化。

---

## 优化前

### 原始代码

```c++
// 主循环（section 2）—— 标量递推
limit = size - radius;
for (; pos < limit; ++pos) {
    currentSum -= input[pos - radius - 1];
    currentSum += input[pos + radius];
    output[pos] = currentSum;
}
```

### 验证正确性

```shell
$ ./validate
Validation Successful
```

### 运行 benchmark

```shell
$ cmake --build . --target benchmarkLab
------------------------------------------------------------
Benchmark                  Time             CPU   Iterations
------------------------------------------------------------
bench_partial_sum       14.8 us         14.8 us       188544
```

### Profile

一级分析（Top-Down）：

```shell
$ perf stat --topdown -a taskset -c 0 ./lab
------------------------------------------------------------
Benchmark                  Time             CPU   Iterations
------------------------------------------------------------
bench_partial_sum       14.8 us         14.8 us        47117

 Performance counter stats for 'system wide':

 %  tma_backend_bound %  tma_retiring %  tma_frontend_bound %  tma_bad_speculation
                32.6           38.5                 6.9                    0.9
```

二级分析（toplev.py L2）：

```shell
$ toplev.py --core S0-C0 -l2 --no-desc taskset -c 0 ./lab
core BE               Backend_Bound             % Slots                       56.1
core BE/Core          Backend_Bound.Core_Bound   % Slots                       56.1  <==
```

**瓶颈分析：** Core Bound 56.1%——主循环每次迭代只有 2 次简单运算，但 `currentSum` 的依赖链限制了流水线吞吐。

---

## 优化后

### 优化后代码

用 SSE4.1 intrinsics 一次处理 8 个元素：先计算 8 个差分值，再做向量前缀和，最后加上运行中的 `currentSum`：

```c++
#include <smmintrin.h>  // SSE4.1

// 主循环 — SSE4.1: 每次处理 8 个元素
const uint8_t *subtractPtr = input.data() + pos - radius - 1;
const uint8_t *addPtr = input.data() + pos + radius;
const uint16_t *outputPtr = output.data() + pos;
__m128i current = _mm_set1_epi16(currentSum);

int i = 0;
for (; i + 7 < limit - pos; i += 8) {
    // 1. 计算 8 个差分值: input[i+radius] - input[i-radius-1]
    __m128i sub_u8 = _mm_loadu_si64(subtractPtr + i);
    __m128i sub = _mm_cvtepu8_epi16(sub_u8);       // uint8 → uint16
    __m128i add_u8 = _mm_loadu_si64(addPtr + i);
    __m128i add = _mm_cvtepu8_epi16(add_u8);
    __m128i diff = _mm_sub_epi16(add, sub);

    // 2. 向量前缀和: diff[0], diff[0..1], diff[0..2], ...
    __m128i s = _mm_add_epi16(diff, _mm_slli_si128(diff, 2));
    s = _mm_add_epi16(s, _mm_slli_si128(s, 4));
    s = _mm_add_epi16(s, _mm_slli_si128(s, 8));

    // 3. 加上 currentSum 并存储
    __m128i result = _mm_add_epi16(s, current);
    _mm_storeu_si128((__m128i *)(outputPtr + i), result);

    // 4. 更新 currentSum 为最后一个元素的值
    currentSum = (uint16_t)_mm_extract_epi16(result, 7);
    current = _mm_set1_epi16(currentSum);
}
pos += i;

// 剩余元素：标量循环
for (; pos < limit; ++pos) {
    currentSum -= input[pos - radius - 1];
    currentSum += input[pos + radius];
    output[pos] = currentSum;
}
```

**关键 intrinsics 说明：**

| Intrinsic           | 作用                               |
| ------------------- | ---------------------------------- |
| `_mm_loadu_si64`    | 加载 8 字节（未对齐）              |
| `_mm_cvtepu8_epi16` | 8 个 uint8 → 8 个 uint16（零扩展） |
| `_mm_sub_epi16`     | 8 个 uint16 逐元素减法             |
| `_mm_slli_si128`    | 128 位左移 N 字节（用于前缀和）    |
| `_mm_add_epi16`     | 8 个 uint16 逐元素加法             |
| `_mm_extract_epi16` | 提取第 N 个 uint16 元素            |
| `_mm_storeu_si128`  | 存储 128 位（未wen dang对齐）      |

### 验证正确性

```shell
$ ./validate
Validation Successful
```

### 运行 benchmark

```shell
$ cmake --build . --target benchmarkLab
------------------------------------------------------------
Benchmark                  Time             CPU   Iterations
------------------------------------------------------------
bench_partial_sum       13.0 us         13.0 us       216013
```

### Profile
wen dang
一级分析（Top-Down）：

```shell
$ perf stat --topdown -a taskset -c 0 ./lab
------------------------------------------------------------
Benchmark                  Time             CPU   Iterations
------------------------------------------------------------
bench_partial_sum       13.0 us         13.0 us        54005

 Performance counter stats for 'system wide':

 %  tma_backend_bound %  tma_retiring %  tma_frontend_bound %  tma_bad_speculation
                26.5           16.6                 7.0                    0.9
```

二级分析（toplev.py L2）：

```shell
$ toplev.py --core S0-C0 -l2 --no-desc taskset -c 0 ./lab
core BE               Backend_Bound  wen dang           % Slots                       82.0
core BE/Core          Backend_Bound.Core_Bound   % Slots                       63.5  <==
```

---

## 优化分析

### 性能对比

| 指标           | 优化前（标量） | 优化后（SSE4.1） | 变化           |
| -------------- | -------------- | ---------------- | -------------- |
| benchmark 耗时 | 14.8 μs        | 13.0 μs          | **1.14x 加速** |
| Core Bound     | 56.1%          | 63.5%            | +7.4%          |
| Backend Bound  | 56.1%          | 82.0%            | +25.9%         |

### 为什么加速只有 1.14x

1. **算法本身高效**：标量版本每次迭代只有 2wen dang 次整数运算（add + subtract），已经是 O(N) 复杂度。SIMD 版本虽然一次处理 8 个元素，但前缀和计算需要 3 次 shift + 3 次 add，开销不小。

2. **数据规模小**：40000 个元素，主循环约 39974 次迭代。SIMD 版本约 4997 次迭代，但每次迭代的指令数更多。

3. **依赖链仍在**：前缀和计算本身有依赖（`s = diff + slli(diff, 2)` 依赖 diff），且 `currentSum` 仍需跨迭代传递。

4. **Core Bound 上升（56.1% → 63.5%）**：SIMD 指令的前缀和计算增加了执行单元的压力。

### 前缀和示意

```
输入: diff = [d0, d1, d2, d3, d4, d5, d6, d7]

Step 1: s = diff + slli(diff, 2)   // 左移 1 个元素 (2 字节)
  s = [d0, d0+d1, d1+d2, d2+d3, d3+d4, d4+d5, d5+d6, d6+d7]

Step 2: s = s + slli(s, 4)         // 左移 2 个元素 (4 字节)
  s = [d0, d0+d1, d0+d1+d2, d0+d1+d2+d3, ...]

Step 3: s = s + slli(s, 8)         // 左移 4 个元素 (8 字节)
  s = [d0, d0+d1, d0+d1+d2, ..., d0+d1+...+d7]
     = 8 个前缀和 ✓
```

### 何时使用 Compiler Intrinsics

| 条件                   | 本场景             | 说明                         |
| ---------------------- | ------------------ | ---------------------------- |
| 编译器无法自动向量化   | ✓ 循环依赖         | `currentSum` 跨迭代依赖      |
| 算法有 SIMD 友好的重构 | ✓ 差分 + 前缀和    | 将依赖链转为可并行的差分计算 |
| 标量版本已是瓶颈       | ✓ Core Bound 56.1% | 执行单元受限                 |
| 数据规模足够大         | △ 40000 元素       | 中等规模，SIMD 收益有限      |
