# SW Memory Prefetching

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

哈希表（`std::vector<int>` 开放寻址，32M 桶）做 1M 次随机查找。每次查找访问 `m_vector[val % N_Buckets]`，地址随机 → 硬件预取器无法预测 → 每次访问大概率 L3 miss。

```
哈希表 m_vector (32M × 4B = 128MB)：
  ┌─────────────────────────────────────────┐
  │ bucket 0 │ bucket 1 │ ... │ bucket 32M-1│
  └─────────────────────────────────────────┘
       ↑ 查找 val=99999 → bucket = 99999 % N
       ↑ 查找 val=123   → bucket = 123
       ↑ 随机跳转，硬件预取器无法预测
```

---

## 优化前

### 原始代码

```c++
// solution.cpp
int solution(const hash_map_t *hash_map, const std::vector<int> &lookups) {
    int result = 0;

    for (int val : lookups) {
        if (hash_map->find(val))
        result += getSumOfDigits(val);
    }

    return result;
}

// solution.hpp — hash_map_t::find
bool find(int val) const {
    int bucket = val % N_Buckets;
    return m_vector[bucket] != UNUSED;
}
```

**问题：** `m_vector[bucket]` 访问地址随机，CPU 无法预取，每次 find() 大概率触发 L3 cache miss（~40-80ns 延迟）。1M 次查找 × ~50ns = ~50ms，与实测吻合。

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
bench1           46.4 ms         46.3 ms           61
```

### Profile

一级分析（Top-Down）：

```shell
$ perf stat --topdown -a taskset -c 0 ./lab
-----------------------------------------------------
Benchmark           Time             CPU   Iterations
-----------------------------------------------------
bench1           45.9 ms         45.9 ms           15

 Performance counter stats for 'system wide':

 %  tma_backend_bound %  tma_retiring %  tma_frontend_bound %  tma_bad_speculation
                33.9            6.4                 9.5                   14.3
```

二级分析（toplev.py L2）：

```shell
$ toplev.py --core S0-C0 -l2 --no-desc taskset -c 0 ./lab
core BAD              Bad_Speculation                     % Slots                       15.3
core BE               Backend_Bound                       % Slots                       72.5
core BAD              Bad_Speculation.Branch_Mispredicts  % Slots                       15.3
core BE/Mem           Backend_Bound.Memory_Bound          % Slots                       48.2  <==
core BE/Core          Backend_Bound.Core_Bound            % Slots                       24.3
```

**瓶颈分析：** Memory Bound 48.2%，CPU 大量时间在等待内存数据返回。随机访问模式使硬件预取器完全失效，每次 `m_vector[bucket]` 访问都需要从 L3 甚至内存加载。Bad Speculation 15.3% 来自 `find()` 中的分支预测失败（值存在/不存在各 50% 概率）。

---

## 优化后

### 优化后代码

在 `hash_map_t` 中添加 `prefetchForVal` 方法，利用 `__builtin_prefetch` 提前发起内存请求：

```c++
// solution.hpp — 新增预取方法
class hash_map_t {
    // ...
    void prefetchForVal(int val) const {
        int bucket = val % N_Buckets;
        __builtin_prefetch(&m_vector[bucket]);
    }
};
```

```c++
// solution.cpp — 带预取的查找
int solution(const hash_map_t *hash_map, const std::vector<int> &lookups) {
    int result = 0;
    constexpr int look_ahead = 16;

    // 主循环：查找当前值 + 预取 look_ahead 步之后的值
    for (size_t i = 0; i < lookups.size() - look_ahead; i++) {
        int val = lookups[i];
        if (hash_map->find(val)) {
            result += getSumOfDigits(val);
        }
        hash_map->prefetchForVal(lookups[i + look_ahead]);
    }

    // 尾部：剩余不足 look_ahead 的元素，无预取
    for (size_t i = lookups.size() - look_ahead; i < lookups.size(); i++) {
        int val = lookups[i];
        if (hash_map->find(val)) {
            result += getSumOfDigits(val);
        }
    }

    return result;
}
```

**原理：** `__builtin_prefetch` 向 CPU 发出硬件预取指令（PREFETCHT0），将目标地址数据拉入 L1 缓存。look_ahead=16 意味着提前 16 次迭代发起预取，给内存子系统足够的延迟窗口（16 × ~10ns 计算 ≈ 160ns > L3 延迟 ~40ns）。

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
bench1           15.0 ms         15.0 ms          191
```

### Profile

一级分析（Top-Down）：

```shell
$ perf stat --topdown -a taskset -c 0 ./lab
-----------------------------------------------------
Benchmark           Time             CPU   Iterations
-----------------------------------------------------
bench1           15.0 ms         15.0 ms           47

 Performance counter stats for 'system wide':

 %  tma_backend_bound %  tma_retiring %  tma_frontend_bound %  tma_bad_speculation
                31.0           10.3                10.7                   15.0
```

二级分析（toplev.py L2）：

```shell
$ toplev.py --core S0-C0 -l2 --no-desc taskset -c 0 ./lab
core BAD              Bad_Speculation                     % Slots                       17.3
core BE               Backend_Bound                       % Slots                       68.5
core BAD              Bad_Speculation.Branch_Mispredicts  % Slots                       17.3
core BE/Mem           Backend_Bound.Memory_Bound          % Slots                       42.1  <==
core BE/Core          Backend_Bound.Core_Bound            % Slots                       26.4
```

---

## 优化分析

### 性能对比

| 指标            | 优化前  | 优化后  | 变化          |
| --------------- | ------- | ------- | ------------- |
| benchmark 耗时  | 46.4 ms | 15.0 ms | **3.1x 加速** |
| Memory Bound    | 48.2%   | 42.1%   | -6.1%         |
| Core Bound      | 24.3%   | 26.4%   | +2.1%         |
| Bad Speculation | 15.3%   | 17.3%   | +2.0%         |
| Retiring        | 6.4%    | 10.3%   | +3.9%         |

### 为什么有效

1. **延迟隐藏**：预取指令在当前元素的 `find()` + `getSumOfDigits()` 计算期间（~10ns × 多次操作）并行执行内存请求。等轮到 look_ahead 步后的元素时，数据已在 L1 缓存中。

2. **look_ahead 的选择**：太小 → 预取来不及完成；太大 → 预取的数据被后续 cache 污染驱逐。16 是经验值，约覆盖 L3 访问延迟（~40ns）除以单次迭代计算时间（~10ns）。

3. **Retiring 提升**：从 6.4% → 10.3%，说明 CPU 做了更多有效工作（cache 命中后指令能正常完成），而非空等内存。

### 预取时序示意

```
时间轴 →
迭代 i:    [find(lookups[i])]  [getSumOfDigits]  [prefetch lookups[i+16]]
                                                         ↓
迭代 i+16:                              [find(lookups[i+16])] ← 数据已在 L1!
                                          ↑ 无 cache miss，直接命中
```

### 软件预取的适用条件

| 条件             | 本场景             | 说明             |
| ---------------- | ------------------ | ---------------- |
| 访问模式不可预测 | ✓ 随机哈希         | 硬件预取器失效   |
| 有足够计算间隔   | ✓ getSumOfDigits   | 预取延迟可被隐藏 |
| 数据量超出缓存   | ✓ 128MB >> L3 36MB | 无法全部缓存     |
| 可提前知道地址   | ✓ lookups[i+16]    | 预取窗口足够     |

> **注意：** 软件预取不是万能的。如果访问模式有规律（如顺序扫描），硬件预取器已足够高效，手动预取反而增加指令开销。仅在随机访问 + 有足够计算间隔时使用。
