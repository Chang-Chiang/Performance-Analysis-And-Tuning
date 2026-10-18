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

---

## 附录：内存对齐与字节填充

### 为什么需要内存对齐

CPU 访问内存时，按对齐边界读取效率最高：

```
未对齐访问: 可能需要 2 次内存访问 + 额外拼接
对齐访问:   1 次内存访问直接完成
```

现代 CPU 的内存总线按固定宽度（通常 8 字节）传输数据。当数据跨越总线边界时，硬件需要拆分成两次传输再拼接，产生额外开销。

### 对齐规则

> **数据的起始地址必须是其大小的整数倍**

| 类型 | 大小 | 起始地址必须是 |
|------|------|----------------|
| `bool` | 1B | 1 的倍数（任意地址） |
| `short` | 2B | 2 的倍数 |
| `int` | 4B | 4 的倍数 |
| `long long` | 8B | 8 的倍数 |
| `double` | 8B | 8 的倍数 |

### 编译器自动对齐

内存对齐是 **编译器自动完成的**。编译器根据目标平台的 ABI（Application Binary Interface）规范，自动在结构体成员之间插入填充字节：

```cpp
// 程序员写的代码
struct S {
    int i;        // 4B
    long long l;  // 8B
    bool b;       // 1B
};

// 编译器实际生成的布局
// [i:4B][填充:4B][l:8B][b:1B][尾部填充:7B] = 24B
```

**编译器不会自动重排字段顺序**，因为顺序是语义的一部分。因此优化填充需要程序员手动调整。

需要手动干预对齐的场景：

| 场景 | 做法 |
|------|------|
| 减少内存占用 | 调整字段顺序（大字段靠前） |
| 网络协议/文件格式 | `#pragma pack` 或 `__attribute__((packed))` |
| SIMD 指令要求 | 手动对齐到 16/32/64 字节 |
| 跨平台二进制兼容 | 显式指定对齐方式 |

### 未对齐访问示例

```cpp
#include <iostream>
#include <cstdint>

int main() {
    // 分配一块内存，起始地址未对齐
    alignas(8) char buffer[16];
    char* base = buffer + 1;  // 偏移 1 字节，未对齐到 8 字节边界

    // 未对齐写入
    long long* ptr = reinterpret_cast<long long*>(base);
    *ptr = 0x123456789ABCDEF0;

    std::cout << "写入值: 0x" << std::hex << *ptr << "\n";
    return 0;
}
```

**未对齐访问的问题：**

```
内存布局 (假设 base = 0x7fff0001，未 8 字节对齐):

地址:     0x7fff0001  0x7fff0002 ... 0x7fff0008
          [-------- long long (8B) --------]
             ↑
             跨越了 8 字节边界！

CPU 需要:
  1. 读取 [0x7fff0000, 0x7fff0007] 的 8 字节
  2. 读取 [0x7fff0008, 0x7fff000f] 的 8 字节
  3. 拼接出实际需要的 8 字节
```

**不同架构的行为：**

| 架构 | 未对齐访问行为 |
|------|----------------|
| x86/x64 | 允许，但性能下降（1.5~2x 慢） |
| ARM (旧版) | 硬件异常 (SIGBUS) |
| ARM (新版) | 允许，但有性能惩罚 |
| RISC-V | 取决于实现，可能异常或性能下降 |
| GPU (CUDA) | 未对齐访问严重影响性能 |

**性能对比：**

```cpp
// 对齐访问 — 1 次内存操作
alignas(8) long long aligned_var;    // 地址: 0x1000
// 读取: 直接从 0x1000 取 8 字节

// 未对齐访问 — 2 次内存操作 + 拼接
long long* unaligned_ptr = reinterpret_cast<long long*>(0x1003);
// 读取: 从 0x1000 取 8 字节 + 从 0x1008 取 8 字节 → 拼接
```

### 字段间填充

以原始结构体为例，假设起始地址为 `0x00`：

```cpp
struct S {
    int i;          // 4B
    long long l;    // 8B
    short s;        // 2B
    double d;       // 8B
    bool b;         // 1B
};
```

```
偏移   字段    大小    布局说明
─────────────────────────────────────────
0x00   i       4B      int 需要 4 字节对齐，0x00 ✓
0x04   ---     4B      填充！l 需要 8 字节对齐，下一个 8 的倍数是 0x08
0x08   l       8B      long long 需要 8 字节对齐，0x08 ✓
0x10   s       2B      short 需要 2 字节对齐，0x10 ✓
0x12   ---     6B      填充！d 需要 8 字节对齐，下一个 8 的倍数是 0x18
0x18   d       8B      double 需要 8 字节对齐，0x18 ✓
0x20   b       1B      bool 需要 1 字节对齐，任意地址 ✓
0x21   ---     7B      填充！结构体总大小必须是最大成员(8B)的倍数
0x28           共 40 字节
```

### 结构体尾部填充

结构体本身的对齐要求等于其最大成员的对齐要求。因此，结构体总大小必须是最大成员大小的整数倍：

```
结构体对齐 = max(各成员对齐) = 8B (本例)
结构体大小 = 向上取整到 8 的倍数
```

本例中 `b` 结束于 `0x21`，下一个 8 的倍数是 `0x28`，因此尾部填充 7 字节。

### 浪费统计

| 类型 | 大小 | 说明 |
|------|------|------|
| 字段间填充 | 10B | `i` 后 4B + `s` 后 6B |
| 结构体尾部 | 7B | 对齐到 8 的倍数 |
| **总浪费** | **17B** | 占 42.5% |
| 有效数据 | 23B | |
| 总大小 | 40B | |

### 验证方法

```cpp
#include <iostream>
#include <cstddef>

struct S {
    int i;
    long long l;
    short s;
    double d;
    bool b;
};

int main() {
    std::cout << "sizeof: " << sizeof(S) << "\n";
    std::cout << "i offset: " << offsetof(S, i) << "\n";
    std::cout << "l offset: " << offsetof(S, l) << "\n";
    std::cout << "s offset: " << offsetof(S, s) << "\n";
    std::cout << "d offset: " << offsetof(S, d) << "\n";
    std::cout << "b offset: " << offsetof(S, b) << "\n";
}
```

输出：

```
sizeof: 40
i offset: 0
l offset: 8
s offset: 16
d offset: 24
b offset: 32
```

### 优化：调整字段顺序

将大对齐要求的字段放前面，可消除大部分填充：

```cpp
struct Optimized {
    long long l;    // 8B
    double d;       // 8B
    int i;          // 4B
    short s;        // 2B
    bool b;         // 1B
};
// 8 + 8 + 4 + 2 + 1 = 23B → 只需 1B 尾部填充 → 24B
```

```
偏移   字段    大小
──────────────────────
0x00   l       8B      ✓ 8 对齐
0x08   d       8B      ✓ 8 对齐
0x10   i       4B      ✓ 4 对齐
0x14   s       2B      ✓ 2 对齐
0x16   b       1B      ✓ 1 对齐
0x17   ---     1B      尾部填充到 8 的倍数
0x18           共 24 字节
```

**节省**: 40B → 24B，减少 40% 内存占用。

### 编译器指令：`#pragma pack`

可通过预处理指令修改默认对齐方式：

```cpp
#pragma pack(push, 1)  // 设置对齐为 1 字节
struct Packed {
    int i;          // 4B
    long long l;    // 8B
    short s;        // 2B
    double d;       // 8B
    bool b;         // 1B
};                  // 总共 23B，无填充
#pragma pack(pop)   // 恢复默认对齐
```

> ⚠️ 谨慎使用：强制取消对齐可能导致未对齐访问，在某些架构上引发性能下降甚至崩溃（如 ARM 上的 SIGBUS）。

### CPU 缓存层级

现代 CPU 采用多级缓存架构，容量从上到下递增，延迟从上到下递增：

```
┌─────────────────────────────────────────────────────────┐
│  寄存器     │  ~1 周期   │  容量极小                     │
├─────────────────────────────────────────────────────────┤
│  L1 缓存    │  ~4 周期   │  32~64 KB，每核独享           │
├─────────────────────────────────────────────────────────┤
│  L2 缓存    │  ~12 周期  │  256 KB~1 MB，每核独享        │
├─────────────────────────────────────────────────────────┤
│  L3 缓存    │  ~40 周期  │  8~32 MB，多核共享            │
├─────────────────────────────────────────────────────────┤
│  主内存     │  ~100 周期 │  数 GB，无容量限制            │
└─────────────────────────────────────────────────────────┘
```

**缓存命中**：数据在缓存中找到 → 快速访问  
**缓存未命中 (cache miss)**：数据不在缓存 → 需从下一级或主内存加载 → 慢

### L3 缓存命中率提升的原因

L3 缓存大小固定（通常 8~32MB），数据越小，能放入缓存的比例越高：

```
L3 缓存假设: 16MB

优化前: sorted 数组 40MB → 只能缓存 40% → 60% 访问 miss
优化后: sorted 数组 8MB  → 全部放入缓存 → 命中率接近 100%
```

**数据量对比：**

```cpp
// 100 万个元素的排序数组
std::vector<S> sorted(N);  // N = 1,000,000

// 优化前: 1,000,000 × 40B = 40MB
// 优化后: 1,000,000 × 8B = 8MB
```

**缓存行为对比：**

| 指标 | 优化前 (40MB) | 优化后 (8MB) |
|------|---------------|--------------|
| 数据大小 | 40MB | 8MB |
| L3 容量 (假设 16MB) | 装不下 | 装得下 |
| 遍历时缓存行为 | 频繁换入换出 | 数据常驻缓存 |
| 内存访问延迟 | ~100 周期 (DRAM) | ~40 周期 (L3) |

**遍历过程示意：**

```
优化前 (40MB，超出 L3):
──────────────────────────────────────────────────
遍历方向 →

[第1部分 16MB]  [第2部分 16MB]  [第3部分 8MB]
     ↓              ↓              ↓
   L3 命中      L3 miss         L3 miss
  (第1部分被    (第1部分被      (前面的又被
   装入缓存)     换出缓存)       换出缓存)

优化后 (8MB，完全放入 L3):
──────────────────────────────────────────────────
遍历方向 →

[       全部 8MB 在 L3 中       ]
     ↓
  全部命中！无需访问 DRAM
```

> **一句话总结**：数据量 < L3 容量 → 数据常驻缓存 → 访问延迟从 DRAM 的 ~100 周期降到 L3 的 ~40 周期。

### 缓存行与对齐的关系

缓存行大小（通常 64B）决定了缓存一次加载的数据量。结构体越小，单个缓存行能容纳的对象越多：

| 结构体大小 | 每缓存行对象数 | 1M 对象需缓存行数 |
|------------|----------------|-------------------|
| 40B | 1 个（浪费 24B） | ~62,500 |
| 24B | 2 个（浪费 16B） | ~41,667 |
| 8B | 8 个 | 12,500 |

减少填充 → 提高缓存利用率 → 减少 cache miss → 提升性能。
