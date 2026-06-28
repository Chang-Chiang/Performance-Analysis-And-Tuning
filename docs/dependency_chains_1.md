# Dependency Chains (Linked List Lookup)

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

两个各有 10000 个节点的链表 l1 和 l2（无重复，随机顺序），在 l2 中查找 l1 的每个值，找到则累加其数位和。

```
链表遍历的本质问题 — 指针追踪依赖链：

  l2 = l2->next  →  必须等当前节点加载完成才能获取下一个节点地址
  ↓
  每次 l2->next 需要一次内存加载（~4-5ns L1 hit, ~12ns L2, ~40ns L3）
  ↓
  遍历 10000 个节点 = 10000 次串行内存加载
```

---

## 优化前

### 原始代码

```c++
unsigned solution(List *l1, List *l2) {
  unsigned retVal = 0;
  List *head2 = l2;

  // 对 l1 的每个值，遍历整个 l2 查找
  while (l1) {
    unsigned v = l1->value;
    l2 = head2;
    while (l2) {
      if (l2->value == v) {
        retVal += getSumOfDigits(v);
        break;
      }
      l2 = l2->next;  // 指针追踪依赖链
    }
    l1 = l1->next;
  }
  return retVal;
}
```

**问题：** O(N²) 算法，l2 被完整遍历 N=10000 次。每次遍历 l2 都是 10000 步的指针追踪依赖链，且 l2 的数据在后续遍历时可能已被驱逐出缓存。

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
bench1           59.7 ms         59.7 ms           47
```

### Profile

一级分析（Top-Down）：

```shell
$ perf stat --topdown -a taskset -c 0 ./lab
-----------------------------------------------------
Benchmark           Time             CPU   Iterations
-----------------------------------------------------
bench1           59.2 ms         59.2 ms           12

 Performance counter stats for 'system wide':

 %  tma_backend_bound %  tma_retiring %  tma_frontend_bound %  tma_bad_speculation
                32.6            9.3                 8.8                    0.7
```

二级分析（toplev.py L2）：

```shell
$ toplev.py --core S0-C0 -l2 --no-desc taskset -c 0 ./lab
core BE               Backend_Bound               % Slots                       87.1
core BE/Mem           Backend_Bound.Memory_Bound   % Slots                       55.7  <==
core BE/Core          Backend_Bound.Core_Bound     % Slots                       31.4
```

**瓶颈分析：** Memory Bound 55.7%——CPU 大部分时间在等待链表节点的内存加载。每次 `l2 = l2->next` 都依赖前一次加载的结果（指针追踪依赖链），CPU 无法提前预取下一个节点。

---

## 优化后

### 优化后代码

从 l1 一次读取 M=4 个值，然后遍历 l2 一次同时查找这 4 个值。l2 的遍历次数从 N 减少到 N/M：

```c++
template <int M>
unsigned solutionM(List *l1, List *l2) {
  unsigned retVal = 0;
  List *head2 = l2;

  int length1 = 0;
  for (List *p = l1; p; p = p->next) length1++;

  // 每次从 l1 取 M 个值
  for (int i = 0; i < length1 / M; i++) {
    std::array<unsigned, M> vals;
    for (int j = 0; j < M; j++) {
      vals[j] = l1->value;
      l1 = l1->next;
    }

    // 遍历 l2 一次，同时查找 M 个值
    l2 = head2;
    int found = 0;
    while (l2) {
      for (int j = 0; j < M; j++) {
        if (l2->value == vals[j]) {
          retVal += getSumOfDigits(l2->value);
          if (++found == M) break;
        }
      }
      if (found == M) break;
      l2 = l2->next;
    }
  }

  // 剩余元素：逐个查找
  while (l1) { /* ... 标量版本 ... */ }

  return retVal;
}

unsigned solution(List *l1, List *l2) {
  return solutionM<4>(l1, l2);
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
-----------------------------------------------------
Benchmark           Time             CPU   Iterations
-----------------------------------------------------
bench1           16.6 ms         16.6 ms          168
```

### Profile

一级分析（Top-Down）：

```shell
$ perf stat --topdown -a taskset -c 0 ./lab
-----------------------------------------------------
Benchmark           Time             CPU   Iterations
-----------------------------------------------------
bench1           16.6 ms         16.6 ms           42

 Performance counter stats for 'system wide':

 %  tma_backend_bound %  tma_retiring %  tma_frontend_bound %  tma_bad_speculation
                39.7           44.9                23.8                    1.6
```

二级分析（toplev.py L2）：

```shell
$ toplev.py --core S0-C0 -l2 --no-desc taskset -c 0 ./lab
core FE               Frontend_Bound            % Slots                       18.0
core BE               Backend_Bound             % Slots                       27.8
core BE/Core          Backend_Bound.Core_Bound   % Slots                       27.5  <==
```

---

## 优化分析

### 性能对比

| 指标 | 优化前 | 优化后 (M=4) | 变化 |
|------|--------|-------------|------|
| benchmark 耗时 | 59.7 ms | 16.6 ms | **3.6x 加速** |
| Memory Bound | 55.7% | — | 大幅下降 |
| Core Bound | 31.4% | 27.5% | -3.9% |
| Retiring | 9.3% | 44.9% | **+35.6%** |
| l2 遍历次数 | 10000 | 2500 | **4x 减少** |

### 为什么有效

1. **l2 遍历次数减少 4x**：原始版本对 l1 的每个值都完整遍历 l2（10000 次）。优化版每次取 4 个值共享一次 l2 遍历，总遍历次数 = 10000/4 = 2500 次。

2. **缓存友好**：l2 的 10000 个节点（~160KB）在 2500 次遍历中更容易保持在 L2/L3 缓存中。原始版本 10000 次遍历时，l2 数据可能被反复驱逐和重新加载。

3. **Retiring 大幅提升（9.3% → 44.9%）**：优化前 CPU 大部分时间在等待内存（Memory Bound 55.7%），有效指令占比极低。优化后等待时间减少，CPU 能做更多有效工作。

4. **指针追踪依赖链的特性**：`l2 = l2->next` 是不可消除的依赖链——每次加载依赖前一次的地址。唯一的优化方式是减少遍历次数或让缓存命中率更高。

### M 的选择

| M | l2 遍历次数 | vals 数组大小 | 权衡 |
|---|------------|--------------|------|
| 1 | 10000 | 4B | 原始版本 |
| 2 | 5000 | 8B | 缓存友好 |
| 4 | 2500 | 16B | 最佳平衡 |
| 8 | 1250 | 32B | l2 遍历内循环变长 |
| 16 | 625 | 64B | 内循环开销可能抵消收益 |

M=4 是经验值：vals 数组（16B）可放在寄存器中，l2 遍历内循环的 4 次比较开销适中。
