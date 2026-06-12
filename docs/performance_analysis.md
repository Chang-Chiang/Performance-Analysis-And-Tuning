# 性能分析测量

- [性能分析测量](#性能分析测量)
  - [性能分析工具分类](#性能分析工具分类)
    - [计数器类（Counter-based）](#计数器类counter-based)
    - [采样类（Sampling-based）](#采样类sampling-based)
    - [追踪类（Tracing-based）](#追踪类tracing-based)
    - [插桩类（Instrumentation-based）](#插桩类instrumentation-based)
    - [模拟类（Simulation-based）](#模拟类simulation-based)
    - [静态分析类（Static analysis）](#静态分析类static-analysis)
  - [工具详解](#工具详解)
    - [工具对比总览](#工具对比总览)
    - [Intel VTune Profiler](#intel-vtune-profiler)
      - [支持的分析类型](#支持的分析类型)
      - [常用操作](#常用操作)
      - [GUI 使用](#gui-使用)
      - [适用场景](#适用场景)
    - [Intel Advisor](#intel-advisor)
      - [核心分析功能](#核心分析功能)
      - [常用操作](#常用操作-1)
      - [Roofline 分析](#roofline-分析)
      - [适用场景](#适用场景-1)
    - [Perf](#perf)
      - [核心子命令](#核心子命令)
      - [perf stat](#perf-stat)
      - [perf record + perf report](#perf-record--perf-report)
      - [perf top](#perf-top)
      - [perf trace](#perf-trace)
      - [常用事件](#常用事件)
      - [Flame Graph（火焰图）](#flame-graph火焰图)
      - [适用场景](#适用场景-2)
    - [其他常用工具](#其他常用工具)
      - [gprof](#gprof)
      - [Valgrind](#valgrind)
      - [ftrace / trace-cmd](#ftrace--trace-cmd)
      - [strace / ltrace](#strace--ltrace)

---

## 性能分析工具分类

### 计数器类（Counter-based）

- **原理**：利用硬件/软件性能计数器，统计特定事件发生次数
- **特点**：开销低，适合宏观分析
- **工具**：`perf stat`, `AMD uProf`, `Intel VTune Profiler`

### 采样类（Sampling-based）

- **原理**：定时中断采样，记录程序执行位置
- **特点**：开销适中，能定位热点函数/代码行
- **工具**：`perf record`, `gprof`, `Intel VTune Profiler`

### 追踪类（Tracing-based）

- **原理**：记录程序执行过程中的详细事件序列
- **特点**：开销较高，能分析执行流程和时序
- **工具**：`perf trace`, `ftrace`, `strace`, `ltrace`, `Intel VTune Profiler`

### 插桩类（Instrumentation-based）

- **原理**：在代码中插入探针，收集详细运行时信息
- **特点**：开销最高，信息最详细
- **工具**：`Valgrind`, `Intel Advisor`, `Google Performance Tools`, `gprof`

### 模拟类（Simulation-based）

- **原理**：模拟硬件行为，预测程序性能
- **特点**：无需实际运行，可分析微架构细节
- **工具**：`gem5`, `Sniper`, `SimpleScalar`

### 静态分析类（Static analysis）

- **原理**：不运行程序，通过分析代码结构预测性能
- **特点**：零运行时开销，适合早期优化
- **工具**：编译器分析报告、`LLVM opt`、性能建模工具

---

## 工具详解

### 工具对比总览

| 工具                 | 分析类型         | 运行开销     | 平台          | 主要用途               |
| -------------------- | ---------------- | ------------ | ------------- | ---------------------- |
| Intel VTune Profiler | 计数器/采样/追踪 | 低~中        | Linux/Win/Mac | CPU/内存/线程综合分析  |
| Intel Advisor        | 插桩             | 中~高        | Linux/Win     | 向量化/Roofline 分析   |
| Perf                 | 计数器/采样/追踪 | 低~中        | Linux         | 系统级通用分析、火焰图 |
| gprof                | 插桩(编译时)     | 低           | 跨平台        | 函数级调用图           |
| Valgrind             | 插桩(二进制)     | 极高(10-50x) | Linux/Mac     | 内存错误/缓存模拟      |
| ftrace/trace-cmd     | 追踪             | 低           | Linux(内核态) | 内核函数追踪           |
| strace/ltrace        | 追踪             | 中           | Linux         | 系统调用/库调用追踪    |

### Intel VTune Profiler

Intel VTune Profiler 是 Intel 官方提供的综合性性能分析工具，支持 Linux、Windows 和 macOS，覆盖了计数器、采样、追踪三大类分析方式。

#### 支持的分析类型

| 分析类型                                        | 说明                                                      |
| ----------------------------------------------- | --------------------------------------------------------- |
| **Hotspots（热点分析）**                        | 基于采样定位 CPU 时间消耗最多的函数和代码行               |
| **Microarchitecture Exploration（微架构探索）** | 利用 PMU 计数器分析 Pipeline、Cache、分支预测等微架构瓶颈 |
| **Memory Access（内存访问）**                   | 分析内存带宽、NUMA 访问模式、Cache Miss                   |
| **Threading（线程分析）**                       | 分析线程并行度、同步开销、负载均衡                        |
| **HPC Performance Characterization**            | 面向 HPC 场景，分析浮点运算效率、向量化率等               |
| **GPU Offload / GPU Compute**                   | 分析 GPU 卸载和计算效率（Intel GPU）                      |

#### 常用操作

```bash
# 命令行采集（以热点分析为例）
vtune -collect hotspots -result-dir ./vtune_result ./your_application

# 命令行查看报告
vtune -report hotspots -result-dir ./vtune_result

# 采集微架构瓶颈
vtune -collect uarch-exploration -result-dir ./vtune_result ./your_application

# 采集内存访问分析
vtune -collect memory-access -result-dir ./vtune_result ./your_application
```

#### GUI 使用

```bash
# 启动 GUI
vtune-gui
```

在 GUI 中可以可视化查看火焰图、时间线、Top-down 微架构分析树等。

#### 适用场景

- **CPU 密集型程序**：热点函数定位、微架构瓶颈分析
- **内存密集型程序**：NUMA 优化、Cache Miss 分析
- **多线程程序**：线程并行效率、锁竞争分析
- **HPC 应用**：浮点效率、向量化率

---

### Intel Advisor

Intel Advisor 是一款面向**向量化优化**和**循环优化**的专业工具，属于插桩类分析，提供比 VTune 更深层次的向量化建议。

#### 核心分析功能

| 分析类型                            | 说明                                                 |
| ----------------------------------- | ---------------------------------------------------- |
| **Survey（概览分析）**              | 扫描程序，识别循环和函数，报告向量化状态和时间占比   |
| **Trip Counts（迭代次数）**         | 收集循环的实际迭代次数，辅助优化决策                 |
| **Dependencies（依赖分析）**        | 检测循环中的数据依赖，判断是否可以安全向量化         |
| **Vectorization（向量化分析）**     | 给出向量化建议，预估向量化后的性能提升               |
| **Roofline Analysis（屋顶线分析）** | 将程序性能与硬件理论峰值对比，定位计算瓶颈或内存瓶颈 |

#### 常用操作

```bash
# 概览分析
advixe-cl --collect survey --project-dir ./advixe_proj -- ./your_application

# 迭代次数分析
advixe-cl --collect tripcounts --project-dir ./advixe_proj -- ./your_application

# 依赖分析
advixe-cl --collect dependencies --project-dir ./advixe_proj -- ./your_application

# 查看报告
advixe-cl --report survey --project-dir ./advixe_proj
```

#### Roofline 分析

Roofline 模型是 Advisor 最强大的功能之一：

```
GFLOPS
  ^
  |        _______________  ← 计算上限 (Compute Roofline)
  |       /
  |      /   ★ 程序实际性能点
  |     /
  |    / ← 内存带宽上限 (Memory Bandwidth Roofline)
  |   /
  +------------------------→ 算术强度 (FLOPS/Byte)
```

- 点在内存带宽斜线上 → **内存瓶颈**，需要优化数据访问模式
- 点在计算平台上 → **计算瓶颈**，需要向量化或算法优化

#### 适用场景

- 循环向量化可行性评估
- 数据依赖检测
- Roofline 性能建模
- 编译器自动向量化效果评估

---

### Perf

Perf 是 Linux 内核自带的性能分析工具，基于 `perf_events` 子系统，覆盖计数器、采样、追踪三大类，是 Linux 平台上最通用的性能分析工具。

#### 核心子命令

| 子命令        | 类别   | 说明                       |
| ------------- | ------ | -------------------------- |
| `perf stat`   | 计数器 | 统计事件发生次数，宏观分析 |
| `perf record` | 采样   | 采样记录，生成 `perf.data` |
| `perf report` | —      | 分析 `perf.data`，查看热点 |
| `perf top`    | 采样   | 实时查看热点函数           |
| `perf trace`  | 追踪   | 类似 strace，追踪系统调用  |
| `perf bench`  | 基准   | 运行内核基准测试           |

#### perf stat

```bash
# 基本统计
perf stat ./your_application

# 指定事件
perf stat -e cache-misses,cache-references,instructions,cycles ./your_application

# 统计多核
perf stat -a -e cycles,instructions sleep 5

# 详细统计（包含更多硬件事件）
perf stat -d ./your_application
```

输出示例：
```
     1,234,567,890      cycles
       987,654,321      instructions      #    0.80  insn per cycle
        12,345,678      cache-misses      #    5.23% of cache-references
        56,789,012      cache-references
       2.345678901 seconds time elapsed
```

关键指标：
- **IPC (insn per cycle)**：每周期指令数，反映 CPU 执行效率
- **Cache Miss Rate**：Cache 未命中率，反映内存访问效率

#### perf record + perf report

```bash
# 采样记录（默认按 CPU 周期采样）
perf record -g ./your_application          # -g 记录调用栈

# 查看报告（交互式）
perf report

# 文本报告
perf report --stdio

# 按指定事件采样
perf record -e cache-misses -g ./your_application
```

#### perf top

```bash
# 实时查看系统热点
sudo perf top

# 指定进程
sudo perf top -p <pid>
```

#### perf trace

```bash
# 追踪系统调用
perf trace ./your_application

# 追踪特定系统调用
perf trace -e read,write ./your_application
```

#### 常用事件

```bash
# 列出可用事件
perf list

# 硬件事件
perf stat -e cycles,instructions,cache-misses,branch-misses ./app

# 软件事件
perf stat -e page-faults,context-switches ./app

# PMU 事件（精确到具体微架构事件）
perf stat -e cpu/event=0xd1,umask=0x01/ ./app   # 示例：L1 Cache Miss
```

#### Flame Graph（火焰图）

```bash
# 采集数据
perf record -F 99 -g ./your_application

# 生成火焰图（需要 FlameGraph 工具）
perf script | stackcollapse-perf.pl | flamegraph.pl > flamegraph.svg
```

火焰图阅读：
- **X 轴**：采样占比（越宽 = 耗时越多）
- **Y 轴**：调用栈深度（越深 = 调用链越长）
- **颜色**：随机，无特殊含义

#### 适用场景

- Linux 系统级性能分析
- 快速定位热点函数
- Cache 和分支预测分析
- 系统调用追踪
- 火焰图生成

---

### 其他常用工具

#### gprof

GNU Profiler，编译时插桩（`-pg` 标志），生成函数级调用图和时间统计。

```bash
# 编译时加 -pg 标志
gcc -pg -o app app.c
./app
gprof app gmon.out > analysis.txt
```

- **开销**：低（编译时插桩）
- **平台**：跨平台（Linux/macOS/Windows）
- **适用场景**：快速了解函数级时间分布和调用关系
- **局限**：精度有限，不支持共享库分析，需要重新编译

#### Valgrind

动态二进制插桩框架，主要用于内存错误检测和缓存模拟。

```bash
# 内存错误检测（最常用）
valgrind --tool=memcheck ./your_application

# 缓存模拟（Cache 命中率分析）
valgrind --tool=cachegrind ./your_application
cg_annotate cachegrind.out.<pid>

# 堆内存分析（内存分配热点）
valgrind --tool=massif ./your_application
ms_print massif.out.<pid>
```

- **开销**：极高（10-50x 慢）
- **平台**：Linux / macOS
- **适用场景**：内存泄漏检测、Cache 行为模拟、堆分配分析
- **局限**：运行极慢，不适合性能 profiling，适合 Debug

#### ftrace / trace-cmd

Linux 内核内置的追踪框架，用于内核函数追踪。`trace-cmd` 是 ftrace 的封装，使用更便捷。

```bash
# 追踪调度事件
sudo trace-cmd record -e sched_switch -e sched_wakeup ./your_application
trace-cmd report

# 追踪特定内核函数的调用图
sudo trace-cmd record -p function_graph -g do_sys_open ./your_application
```

- **开销**：低
- **平台**：Linux（内核态）
- **适用场景**：系统调用追踪、调度分析、中断延迟分析
- **局限**：需要 root 权限，仅限内核态

#### strace / ltrace

系统调用和库调用追踪工具。

```bash
# strace: 追踪系统调用
strace -c ./your_application              # 统计摘要（调用次数和耗时）
strace -e trace=open,read,write ./app     # 过滤特定调用

# ltrace: 追踪动态库函数调用
ltrace ./your_application
ltrace -c ./your_application              # 统计摘要
```

- **开销**：中
- **平台**：Linux
- **适用场景**：I/O 密集型程序的系统调用分析、库函数调用追踪
- **局限**：仅追踪用户态，不涉及内核内部行为
