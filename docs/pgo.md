# PGO (Profile-Guided Optimization)

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

Lua 解释器基准测试。PGO 通过运行时 profiling 数据指导编译器优化，改善分支预测、代码布局和函数内联决策。

```
普通编译 vs PGO 编译：

普通编译：
  源码 → 编译 → 二进制
  编译器靠启发式猜测哪些分支更热、哪些函数该内联

PGO 编译（三步）：
  源码 → 插桩编译 → 插桩二进制 → 运行收集 profile → 带 profile 编译 → 优化二进制
         Step 1                Step 2                    Step 3
```

---

## 优化前

### 基准版本（无 PGO）

CMakeLists.txt 不添加任何 PGO 标志：

```cmake
# 无 PGO 标志
```

### 验证正确性

```shell
$ ./validate ../reference_output.txt
Validation Successful
```

### 运行 benchmark

```shell
$ cmake --build . --target benchmarkLab
-----------------------------------------------------
Benchmark           Time             CPU   Iterations
-----------------------------------------------------
bench1           2988 ms         2987 ms            1
```

### Profile

一级分析（Top-Down）：

```shell
$ perf stat --topdown -a taskset -c 0 ./lab
-----------------------------------------------------
Benchmark           Time             CPU   Iterations
-----------------------------------------------------
bench1           2968 ms         2968 ms            1

 Performance counter stats for 'system wide':

 %  tma_backend_bound %  tma_retiring %  tma_frontend_bound %  tma_bad_speculation
                35.0           50.8                25.8                    9.4
```

二级分析（toplev.py L2）：

```shell
$ toplev.py --core S0-C0 -l2 --no-desc taskset -c 0 ./lab
core FE               Frontend_Bound                  % Slots                       23.4
core FE               Frontend_Bound.Fetch_Bandwidth  % Slots                       20.3  <==
```

**瓶颈分析：** Frontend Bound 23.4%（Fetch Bandwidth 20.3%）——CPU 前端取指带宽受限。Lua 解释器的字节码分发循环（dispatch loop）有大量间接跳转，前端难以高效取指。

---

## 优化后

### Step 1：插桩编译

在 CMakeLists.txt 中启用插桩标志：

```cmake
# GCC 标志
set(CMAKE_C_FLAGS "-fprofile-generate" ${CMAKE_C_FLAGS})

# Clang 标志
# set(CMAKE_C_FLAGS "-fprofile-instr-generate" ${CMAKE_C_FLAGS})
```

```shell
$ cmake -DCMAKE_BUILD_TYPE=Release ..
$ cmake --build . --config Release --parallel 8
```

### Step 2：收集 Profile 数据

运行插桩版本的 benchmark 和 validation，覆盖所有代码路径：

```shell
# 运行 benchmark 收集热路径数据
$ cmake --build . --target benchmarkLab
bench1           3739 ms         3726 ms            1   ← 插桩有开销，正常

# 运行 validation 收集更多代码路径
$ ./validate ../reference_output.txt
Validation Successful
```

GCC 自动生成 `*.gcda` profile 数据文件。

### Step 3：带 Profile 编译

修改 CMakeLists.txt，用收集到的 profile 数据指导编译：

```cmake
# GCC 标志
set(CMAKE_C_FLAGS "-fprofile-use=${CMAKE_CURRENT_SOURCE_DIR}/build_pgo" ${CMAKE_C_FLAGS})

# Clang 标志
# set(CMAKE_C_FLAGS "-fprofile-instr-use=code.profdata" ${CMAKE_C_FLAGS})
```

```shell
$ cmake -DCMAKE_BUILD_TYPE=Release ..
$ cmake --build . --config Release --parallel 8
```

### 验证正确性

```shell
$ ./validate ../reference_output.txt
Validation Successful
```

### 运行 benchmark

```shell
$ cmake --build . --target benchmarkLab
-----------------------------------------------------
Benchmark           Time             CPU   Iterations
-----------------------------------------------------
bench1           3032 ms         3032 ms            1
```

### Profile

一级分析（Top-Down）：

```shell
$ perf stat --topdown -a taskset -c 0 ./lab
-----------------------------------------------------
Benchmark           Time             CPU   Iterations
-----------------------------------------------------
bench1           3012 ms         3011 ms            1

 Performance counter stats for 'system wide':

 %  tma_backend_bound %  tma_retiring %  tma_frontend_bound %  tma_bad_speculation
                29.5           50.3                26.3                    9.6
```

二级分析（toplev.py L2）：

```shell
$ toplev.py --core S0-C0 -l2 --no-desc taskset -c 0 ./lab
core FE               Frontend_Bound  % Slots                       23.0  <==
```

---

## 优化分析

### 性能对比

| 指标 | 无 PGO | 有 PGO | 变化 |
|------|--------|--------|------|
| benchmark 耗时 | 2988 ms | 3032 ms | ~持平 |
| Frontend Bound | 23.4% | 23.0% | -0.4% |
| Backend Bound | 35.0% | 29.5% | -5.5% |

### 为什么 PGO 改善有限

1. **Lua 解释器特性**：核心热点是字节码分发循环（`switch` + 间接跳转），PGO 对间接跳转的优化能力有限。

2. **Release 模式已充分优化**：`-O2`/`-O3` 已经包含了大部分启发式优化，PGO 的额外收益在高度优化的代码上较小。

3. **分支预测已较好**：Lua 的字节码分发循环的分支模式相对规律，硬件分支预测器已能较好处理。

### PGO 改善什么

| 优化项 | 无 PGO | 有 PGO | 说明 |
|--------|--------|--------|------|
| 分支预测 | 编译器猜测 | 基于真实数据 | 热分支放在 fall-through 路径 |
| 代码布局 | 按源码顺序 | 热函数聚集 | 减少 I-cache miss |
| 函数内联 | 基于代码大小 | 基于调用频率 | 热调用点优先内联 |
| 循环优化 | 启发式 | 基于迭代次数 | 热循环优先展开/向量化 |

### PGO 的三步流程

```
Step 1: 插桩编译
  源码 + -fprofile-generate → 插桩二进制
  ↓
  每个分支/函数/循环插入计数器

Step 2: 收集 Profile
  运行插桩二进制 → *.gcda (GCC) / default.profraw (Clang)
  ↓
  记录：分支 A 执行 1000 次，分支 B 执行 10 次

Step 3: 带 Profile 编译
  源码 + -fprofile-use → 优化二进制
  ↓
  编译器知道分支 A 是热路径，将其放在 fall-through 位置
```

### PGO vs LTO

| 特性 | PGO | LTO |
|------|-----|-----|
| 数据来源 | 运行时 profiling | 编译时 IR |
| 改善分支预测 | ✓ 知道哪个分支更热 | ✗ |
| 跨文件内联 | ✗ | ✓ |
| 代码布局优化 | ✓ 热函数聚集 | ✗ |
| 编译流程 | 三步（插桩→运行→编译） | 一步 |
| 可组合 | ✓ PGO + LTO 叠加使用 | ✓ |
