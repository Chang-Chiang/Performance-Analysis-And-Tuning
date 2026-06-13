# Data Packing

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

```c++
struct S {
    int i;
    long long l;
    short s;
    double d;
    bool b;

    bool operator<(const S &s) const { return this->i < s.i; }
};

// check sizeof S during compiling
template <int N>
class TD;
// never compiles but shows the value of sizeof(s)
TD<sizeof(S)> td;
```

### 编译

查看结构体占用字节数：

```shell
$ cmake --build . --config Release --parallel 8
In file included from .../solution.h:23:15: error: aggregate 'TD<40> td' has incomplete type and cannot be defined
   23 | TD<sizeof(S)> td;
```

```
40 bytes
```

原始结构体布局：

| 字段 | 类型 | 大小 | 对齐填充 |
|------|------|------|----------|
| `i` | `int` | 4B | +4B 填充 |
| `l` | `long long` | 8B | — |
| `s` | `short` | 2B | +6B 填充 |
| `d` | `double` | 8B | — |
| `b` | `bool` | 1B | +7B 填充 |
| **合计** | | **40B** | 17B 填充 |

> 40 字节中只有 23 字节是有效数据，17 字节（42.5%）是对齐填充。

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
bench1           30.1 ms         30.1 ms           93
```

### Profile

一级分析（Top-Down）：

```shell
$ perf stat --topdown -a taskset -c 0 ./lab
-----------------------------------------------------
Benchmark           Time             CPU   Iterations
-----------------------------------------------------
bench1           30.1 ms         30.1 ms           23

 Performance counter stats for 'system wide':

 %  tma_backend_bound %  tma_retiring %  tma_frontend_bound %  tma_bad_speculation
                46.0           14.7                12.6                    1.6
```

**瓶颈分析：** Backend Bound 高达 46.0%，说明 CPU 执行单元大量时间在等待数据从内存/缓存返回。根本原因是 40 字节的结构体导致缓存行利用率低，排序时频繁触发 cache miss。

---

## 优化后

### 优化后代码

```c++
struct S {
    float          d;       // double → float，节省 4B
    long long      l : 16;  // 位域压缩
    int            i : 8;   // 位域压缩
    unsigned short s : 7;   // 位域压缩
    bool           b : 1;   // 位域压缩

    bool operator<(const S &s) const { return this->i < s.i; }
};

// check sizeof S during compiling
template <int N>
class TD;
// never compiles but shows the value of sizeof(s)
TD<sizeof(S)> td;
```

### 编译

查看优化后结构体占用字节数：

```shell
$ cmake --build . --config Release --parallel 8
In file included from .../solution.h:33:15: error: aggregate 'TD<8> td' has incomplete type and cannot be defined
   33 | TD<sizeof(S)> td;
```

```
8 bytes
```

优化后结构体布局：

| 字段 | 类型 | 位宽 | 说明 |
|------|------|------|------|
| `d` | `float` | 32b | 原 `double` 降精度，节省 4B |
| `l` | `long long : 16` | 16b | 值域 0~10000，16 位足够 |
| `i` | `int : 8` | 8b | 值域 0~100，8 位足够 |
| `s` | `unsigned short : 7` | 7b | 值域 0~100，7 位足够 |
| `b` | `bool : 1` | 1b | 布尔值，1 位足够 |
| **合计** | | **64b = 8B** | 无填充浪费 |

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
bench1           4.03 ms         4.03 ms          694
```

### Profile

一级分析（Top-Down）：

```shell
$ perf stat --topdown -a taskset -c 0 ./lab
-----------------------------------------------------
Benchmark           Time             CPU   Iterations
-----------------------------------------------------
bench1           4.04 ms         4.04 ms          173

 Performance counter stats for 'system wide':

 %  tma_backend_bound %  tma_retiring %  tma_frontend_bound %  tma_bad_speculation
                38.0           31.8                 8.3                    1.4
```

---

## 优化分析

### 性能对比

| 指标 | 优化前 | 优化后 | 提升 |
|------|--------|--------|------|
| 结构体大小 | 40B | 8B | **5x 缩小** |
| benchmark 耗时 | 30.1 ms | 4.03 ms | **7.5x 加速** |
| 缓存行利用率 | 1.5 个/行 | 8 个/行 | **5.3x 提升** |
| Backend Bound | 46.0% | 38.0% | -8.0% |
| Retiring | 14.7% | 31.8% | +17.1% |
| Frontend Bound | 12.6% | 8.3% | -4.3% |
| Bad Speculation | 1.6% | 1.4% | -0.2% |

### 为什么有效

1. **缓存行利用率**：64B 缓存行从装 1~2 个结构体变为装 8 个，排序遍历时 cache miss 大幅减少。

2. **内存带宽**：排序需要交换元素，40B 意味着每次交换搬运 5 倍数据量。

3. **位域压缩的可行性**：`i` 值域 [0, 100] 只需 7 位，`s` 同理，`l` 最大 100×100=10000 只需 14 位。用位域不会丢失信息。

4. **精度权衡**：`double → float` 精度从 15 位有效数字降至 7 位，对本场景（除以 100）完全够用。

### 排序算法分析

```c++
// solution.cpp — 计数排序
void solution(std::vector<S> &arr) {
    std::shuffle(arr.begin(), arr.end(), g);  // 打乱

    constexpr int cntSize = maxRandom - minRandom + 1;
    std::array<int, cntSize> cnt{};
    for (const auto& v : arr)
        ++cnt[v.i - minRandom + 1];
    for (int i = 1; i < cntSize; ++i)
        cnt[i] += cnt[i - 1];
    std::vector<S> sorted(N);
    for (const auto& v : arr)
        sorted[cnt[v.i - minRandom]++] = v;
    arr = sorted;
}
```

计数排序的瓶颈在于遍历 100 万个元素做 `++cnt` 和 `sorted[cnt[...]++]=v`。结构体缩小后：
- 第一趟遍历：同样 cache line 能预取更多元素
- 第二趟拷贝：`sorted` 数组占用内存从 40MB 降至 8MB，L3 缓存命中率提升
