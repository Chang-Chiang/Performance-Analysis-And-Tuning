# LTO (Link-Time Optimization)

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

环境光遮蔽（Ambient Occlusion）渲染器，代码拆分为 7 个翻译单元：

```
ao.cpp            → 主入口
ao_init.cpp       → 场景初始化
ao_render.cpp     → 渲染主循环
ao_intersect.cpp  → 光线-球体/平面求交
ao_occlusion.cpp  → 环境光遮蔽计算
ao_orthoBasis.cpp → 正交基构造
ao_helpers.cpp    → 向量数学工具
```

**问题：** 每个 `.cpp` 独立编译，编译器看不到跨文件的函数调用关系，无法跨翻译单元内联和优化。

---

## 优化前

### 无 LTO 构建

CMakeLists.txt 中注释掉 `-flto`：

```cmake
# set(CMAKE_CXX_FLAGS "-flto ${CMAKE_CXX_FLAGS}")
```

### 验证正确性

```shell
$ ./lab && cp ao.ppm ../golden_nolto.ppm
$ ./validate ../golden_nolto.ppm
Validation Successful
```

### 运行 benchmark

```shell
$ cmake --build . --target benchmarkLab
-----------------------------------------------------
Benchmark           Time             CPU   Iterations
-----------------------------------------------------
bench1            517 ms          516 ms            5
```

### Profile

一级分析（Top-Down）：

```shell
$ perf stat --topdown -a taskset -c 0 ./lab
-----------------------------------------------------
Benchmark           Time             CPU   Iterations
-----------------------------------------------------
bench1            520 ms          520 ms            1

 Performance counter stats for 'system wide':

 %  tma_backend_bound %  tma_retiring %  tma_frontend_bound %  tma_bad_speculation
                37.3           23.6                10.1                   16.8
```

二级分析（toplev.py L2）：

```shell
$ toplev.py --core S0-C0 -l2 --no-desc taskset -c 0 ./lab
core BAD              Bad_Speculation                     % Slots                       19.7
core BE               Backend_Bound                       % Slots                       50.4
core BAD              Bad_Speculation.Branch_Mispredicts  % Slots                       19.7
core BE/Core          Backend_Bound.Core_Bound            % Slots                       39.4  <==
```

**瓶颈分析：** Core Bound 39.4%——小函数（`vdot`, `vnormalize`, `clamp` 等）的跨文件调用无法内联，每次调用都有函数调用开销（参数传递、栈帧操作、返回值处理）。

---

## 优化后

### LTO 构建

在 CMakeLists.txt 中启用 `-flto`：

```cmake
set(CMAKE_CXX_FLAGS "-flto ${CMAKE_CXX_FLAGS}")
```

LTO 的工作原理：

```
无 LTO 的编译流程：
  ao.cpp        → 编译 → ao.o        ┐
  ao_render.cpp → 编译 → ao_render.o │ 链接 → lab
  ao_helpers.cpp→ 编译 → ao_helpers.o┘
  每个 .o 独立优化，无法跨文件内联

有 LTO 的编译流程：
  ao.cpp        → 编译 → ao.o (含 LLVM IR / GIMPLE) ┐
  ao_render.cpp → 编译 → ao_render.o (含 IR)        │ LTO 链接 → 全局优化 → lab
  ao_helpers.cpp→ 编译 → ao_helpers.o (含 IR)        ┘
  链接时可以看到所有 IR，跨文件内联 + 全局优化
```

### 验证正确性

```shell
$ ./lab && cp ao.ppm ../golden_lto.ppm
$ ./validate ../golden_lto.ppm
Validation Successful
```

> **注意：** LTO 优化可能改变浮点运算的指令调度和寄存器分配，导致渲染结果与无 LTO 版本有微小差异。两个版本各自内部是确定性的，但互相不兼容。这是浮点运算的正常现象。

### 运行 benchmark

```shell
$ cmake --build . --target benchmarkLab
-----------------------------------------------------
Benchmark           Time             CPU   Iterations
-----------------------------------------------------
bench1            344 ms          344 ms            8
```

### Profile

一级分析（Top-Down）：

```shell
$ perf stat --topdown -a taskset -c 0 ./lab
-----------------------------------------------------
Benchmark           Time             CPU   Iterations
-----------------------------------------------------
bench1            344 ms          344 ms            2

 Performance counter stats for 'system wide':

 %  tma_backend_bound %  tma_retiring %  tma_frontend_bound %  tma_bad_speculation
                35.7           22.2                12.7                   29.0
```

二级分析（toplev.py L2）：

```shell
$ toplev.py --core S0-C0 -l2 --no-desc taskset -c 0 ./lab
core BAD              Bad_Speculation                     % Slots                       35.2
core BE               Backend_Bound                       % Slots                       33.2
core BAD              Bad_Speculation.Branch_Mispredicts  % Slots                       35.2  <==
core BE/Core          Backend_Bound.Core_Bound            % Slots                       30.9
```

---

## 优化分析

### 性能对比

| 指标 | 无 LTO | 有 LTO | 变化 |
|------|--------|--------|------|
| benchmark 耗时 | 517 ms | 344 ms | **1.50x 加速** |
| Core Bound | 39.4% | 30.9% | **-8.5%** |
| Bad Speculation | 19.7% | 35.2% | +15.5% |
| Backend Bound | 50.4% | 33.2% | -17.2% |

### 为什么有效

1. **跨翻译单元内联**：LTO 允许编译器在链接时看到所有翻译单元的中间表示（IR），将 `vdot()`, `vnormalize()`, `clamp()` 等小函数内联到调用点，消除函数调用开销。

2. **全局优化**：内联后编译器可以进一步优化——常量传播、死代码消除、循环优化等在更大的代码范围内生效。

3. **Core Bound 下降（39.4% → 30.9%）**：内联消除了函数调用的参数传递和栈帧操作，CPU 执行单元能做更多有效工作。

4. **Bad Speculation 上升（19.7% → 35.2%）**：内联后代码体积增大，分支更多，预测难度增加。但这是"甜蜜的负担"——总耗时仍然大幅下降。

### LTO 的代价

| 代价 | 说明 |
|------|------|
| 编译时间增加 | 链接时需要全局优化，比普通链接慢 |
| 内存用量增加 | 链接时需要加载所有 IR 到内存 |
| 调试更困难 | 内联后栈回溯可能丢失中间帧 |
| 需要编译器支持 | GCC 需要 `ar` / `ranlib` 的 LTO 变体 |

### 何时使用 LTO

| 条件 | 本场景 | 说明 |
|------|--------|------|
| 多翻译单元项目 | ✓ 7 个 .cpp 文件 | LTO 收益大 |
| 小函数跨文件调用 | ✓ vdot, clamp 等 | 内联收益高 |
| 热点函数跨文件 | ✓ render → intersect → occlusion | 关键路径上的调用被内联 |
| 可接受编译时间增加 | ✓ 一次编译 | 运行时收益远大于编译时间代价 |
