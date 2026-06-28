# Function Inlining

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

对 10000 个 `S{key1, key2}` 结构体排序。原始实现使用 C 风格的 `qsort` + 函数指针回调。

```
qsort 的问题：

  qsort(arr, N, sizeof(S), compare)
                            ↓
                    函数指针 compare
                    ↓
  编译器无法内联 compare → 每次比较都有函数调用开销
  编译器无法对 compare 做特化优化（如分支预测提示）
  qsort 本身在 glibc 中 → 跨库调用，指令缓存不友好
```

---

## 优化前

### 原始代码

```c++
static int compare(const void *lhs, const void *rhs) {
  auto &a = *reinterpret_cast<const S *>(lhs);
  auto &b = *reinterpret_cast<const S *>(rhs);

  if (a.key1 < b.key1) return -1;
  if (a.key1 > b.key1) return 1;
  if (a.key2 < b.key2) return -1;
  if (a.key2 > b.key2) return 1;
  return 0;
}

void solution(std::array<S, N> &arr) {
  qsort(arr.data(), arr.size(), sizeof(S), compare);
}
```

**问题：** `compare` 通过函数指针调用，编译器无法内联。10000 个元素排序约需 ~130,000 次比较（O(N log N)），每次比较都有间接调用开销。

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
bench1            558 us          557 us         5021
```

### Profile

一级分析（Top-Down）：

```shell
$ perf stat --topdown -a taskset -c 0 ./lab
-----------------------------------------------------
Benchmark           Time             CPU   Iterations
-----------------------------------------------------
bench1            564 us          564 us         1228

 Performance counter stats for 'system wide':

 %  tma_backend_bound %  tma_retiring %  tma_frontend_bound %  tma_bad_speculation
                25.7           13.6                57.4                   16.1
```

二级分析（toplev.py L2）：

```shell
$ toplev.py --core S0-C0 -l2 --no-desc taskset -c 0 ./lab
core FE               Frontend_Bound                      % Slots                       59.6  <==
core FE               Frontend_Bound.Fetch_Latency        % Slots                       27.8
core FE               Frontend_Bound.Fetch_Bandwidth      % Slots                       31.8
core BAD              Bad_Speculation.Branch_Mispredicts   % Slots                       18.0
```

**瓶颈分析：** Frontend Bound 59.6%——CPU 前端无法及时供给指令。原因：
1. **Fetch Latency 27.8%**：`qsort` 在 glibc 中，每次间接调用 compare 都需要从远端代码区域取指，L1i miss 率高。
2. **Branch Mispredicts 18.0%**：函数指针调用是间接跳转，目标地址不固定，分支预测器难以准确预测。

---

## 优化后

### 优化后代码

用 `std::sort` + lambda 替代 `qsort` + 函数指针：

```c++
void solution(std::array<S, N> &arr) {
  std::sort(arr.begin(), arr.end(), [](const S &a, const S &b) {
    return a.key1 < b.key1 || (a.key1 == b.key1 && a.key2 < b.key2);
  });
}
```

**改进点：**
1. lambda 是编译器可见的类型，`std::sort` 的模板会将比较器**内联**到排序循环中，消除函数调用开销。
2. 编译器可以对内联后的比较逻辑做进一步优化（常量传播、分支合并等）。
3. `std::sort` 使用 introsort（快排+堆排+插入排序），比 glibc 的 `qsort` 更现代。

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
bench1            342 us          342 us         8184
```

### Profile

一级分析（Top-Down）：

```shell
$ perf stat --topdown -a taskset -c 0 ./lab
-----------------------------------------------------
Benchmark           Time             CPU   Iterations
-----------------------------------------------------
bench1            344 us          344 us         2054

 Performance counter stats for 'system wide':

 %  tma_backend_bound %  tma_retiring %  tma_frontend_bound %  tma_bad_speculation
                30.8            6.8                51.3                   25.8
```

二级分析（toplev.py L2）：

```shell
$ toplev.py --core S0-C0 -l2 --no-desc taskset -c 0 ./lab
core FE               Frontend_Bound                      % Slots                       52.5
core FE               Frontend_Bound.Fetch_Latency        % Slots                       11.0
core FE               Frontend_Bound.Fetch_Bandwidth      % Slots                       41.6  <==
core BAD              Bad_Speculation.Branch_Mispredicts   % Slots                       29.0
```

---

## 优化分析

### 性能对比

| 指标 | 优化前 (qsort) | 优化后 (std::sort) | 变化 |
|------|---------------|-------------------|------|
| benchmark 耗时 | 558 μs | 342 μs | **1.63x 加速** |
| Frontend Bound | 59.6% | 52.5% | -7.1% |
| Fetch Latency | 27.8% | 11.0% | **-16.8%** |
| Fetch Bandwidth | 31.8% | 41.6% | +9.8% |
| Bad Speculation | 18.0% | 29.0% | +11.0% |

### 为什么有效

1. **Fetch Latency 大幅下降（27.8% → 11.0%）**：内联消除了间接调用，排序循环的指令都在连续地址空间，L1i 命中率提升。

2. **Bad Speculation 上升（18.0% → 29.0%）**：内联后比较逻辑变复杂（`a.key1 < b.key1 || (a.key1 == b.key1 && a.key2 < b.key2)`），分支更多，预测难度增加。但这些分支的代价远低于间接函数调用的代价。

3. **Fetch Bandwidth 上升（31.8% → 41.6%）**：内联后排序循环的代码体积增大（比较逻辑直接嵌入），前端需要取更多指令。但这是"甜蜜的负担"——说明 CPU 前端在高效工作，而非卡在间接调用上。

### 间接调用 vs 内联

```
优化前 (qsort + 函数指针)：

  qsort 循环体:
    调用 compare(lhs, rhs)    ← 间接调用，目标不确定
    ↓
    保存寄存器 → 跳转到 compare → 执行比较 → 返回 → 恢复寄存器
    ↓
    每次比较 ~10-20 个额外周期（调用开销）

优化后 (std::sort + 内联 lambda)：

  排序循环体:
    a.key1 < b.key1 ?          ← 直接比较，无调用
    a.key1 == b.key1 && a.key2 < b.key2 ?
    ↓
    每次比较 ~2-3 个周期（仅比较指令）
```

### 函数内联的适用条件

| 条件 | 本场景 | 说明 |
|------|--------|------|
| 调用频率极高 | ✓ ~130K 次比较 | 调用开销被放大 |
| 函数体很小 | ✓ 仅 5 行比较 | 内联后代码膨胀小 |
| 通过函数指针/虚函数调用 | ✓ qsort 回调 | 消除间接调用收益大 |
| 编译器可见定义 | ✓ lambda 在同文件 | 模板实例化时自动内联 |
