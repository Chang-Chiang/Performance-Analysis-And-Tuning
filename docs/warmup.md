# Warmup

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

计算 1 到 N 的累加和（N=1000）。这是性能调优的热身练习——展示算法优化比微优化更重要。

```
问题：sum = 1 + 2 + 3 + ... + N = ?

方法 1（循环）：O(N)
  for (i = 1 to N) sum += i

方法 2（公式）：O(1)
  sum = N * (N + 1) / 2
```

---

## 优化前

### 原始代码

```c++
int solution(int* arr, int N) {
    int res = 0;
    for (int i = 0; i < N; i++) {
        res += arr[i];
    }
    return res;
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
bench1           22.9 ns         22.9 ns    120938363
```

### Profile

一级分析（Top-Down）：

```shell
$ perf stat --topdown -a taskset -c 0 ./lab
-----------------------------------------------------
Benchmark           Time             CPU   Iterations
-----------------------------------------------------
bench1           22.9 ns         22.9 ns     30531077

 Performance counter stats for 'system wide':

 %  tma_backend_bound %  tma_retiring %  tma_frontend_bound %  tma_bad_speculation
                30.2           37.7                35.3                   20.8
```

---

## 优化后

### 优化后代码

用高斯公式替代循环，O(N) → O(1)：

```c++
int solution(int* arr, int N) {
    return (N * (N + 1)) / 2;
}
```

### 验证正确性

```shell
$ ./validate
Validation Successful
```

### 运行 benchmarkup

```shell
$ cmake --build . --target benchmarkLab
-----------------------------------------------------
Benchmark           Time             CPU   Iterations
-----------------------------------------------------
bench1          0.325 ns        0.325 ns   8596332102
```

### Profile

一级分析（Top-Down）：

```shell
$ perf stat --topdown -a taskset -c 0 ./lab
-----------------------------------------------------
Benchmark           Time             CPU   Iterations
-----------------------------------------------------
bench1          0.325 ns        0.325 ns   2150358332

 Performance counter stats for 'system wide':

 %  tma_backend_bound %  tma_retiring %  tma_frontend_bound %  tma_bad_speculation
                49.3           70.5                11.3                    1.4
```

---

## 优化分析

### 性能对比

| 指标           | 循环版本 | 公式版本 | 变化         |
| -------------- | -------- | -------- | ------------ |
| benchmark 耗时 | 22.9 ns  | 0.325 ns | **70x 加速** |
| 复杂度         | O(N)     | O(1)     | 算法质变     |
| Retiring       | 37.7%    | 70.5%    | +32.8%       |

### 为什么有效

1. **算法优化 > 微优化**：循环版本需要 1000 次迭代，每次 1 次加法 + 1 次比较 + 1 次分支。公式版本只需 1 次乘法 + 1 次加法 + 1 次除法。

2. **Retiring 大幅提升**：循环版本 37.7% → 公式版本 70.5%。公式版本的 CPU 执行槽几乎都在做有效工作，没有循环开销和分支预测的浪费。

3. **编译器优化**：公式版本中 `arr` 参数完全未使用，编译器可以将其优化掉，甚至整个函数可以被内联为常量。

### 这个练习的启示

```
性能优化的优先级：

  1. 算法优化     ← 本例：O(N) → O(1)，70x 加速
  2. 数据结构优化
  3. 内存布局优化
  4. 微架构优化（SIMD、分支消除等）
  5. 编译器提示（PGO、LTO 等）

永远先问：能不能用更好的算法？
```
