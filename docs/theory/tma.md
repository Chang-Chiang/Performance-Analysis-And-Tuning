## 二. TMA, 性能分析与优化

### 自顶向下微架构分析

TMA: Top-Down Microarchitecture Analysis, 自顶向下微架构分析。

“流水线槽（pipeline slot）”是 Intel Top-Down 模型里人为定义的一个**“最小可观察时间单元”**，用来把“CPU 每周期到底干了多少活”量化成可计数的粒度。一句话：一个 slot 就是“一个物理核心在一个时钟周期内所能发射的一条 μOP 的位置”。

流水线槽划分为四种状态：

- Retiring: uOp 正常退休，表示执行效率高	
- Bad Speculation: uOp 因分支预测错误等被丢弃，浪费资源	
- Frontend Bound: 前端无法提供足够 uOp，流水线空转	
- Backend Bound: 后端资源不足，无法执行 uOp，流水线停滞	

CPU 前端：主要目的是有效地从内存中获取指令并解码，将准备好的指令送入 CPU 后端。

CPU 后端：负责指令的实际执行。

---

![TMA 层级结构](./assets/TMA_hierarchy.jpeg)

TMA 分析流程流程，自顶向下：

1. 一级分类

   - Retiring
   - Bad Speculation
   - Frontend Bound
   - Backend Bound

2. 二级分类

   - Backend Bound
       - Memory Bound, 内存访问延迟
       - Core Bound, 执行单元饱和、指令依赖
   - Frontend Bound
       - Fetch Latency
       - Fetch Bandwidth

3. 三级分类，具体的微架构事件

    - Backend Bound
       - Memory Bound
           - Stores Bound
           - L1 Bound
           - ···
       - Core Bound
           - Divider
           - ···
   - Frontend Bound
       - Fetch Latency
           - iTLB Miss
           - i-Cache Miss
           - Branch Resteers
           - ···
       - Fetch Bandwidth

### Linux Perf 中的 TMA

#### 一级分析

```shell
$ perf stat --topdown -a -- taskset -c 0 ./benchmark
Retiring: 25% | Bad Speculation: 2% | Frontend Bound: 8% | Backend Bound: 65%
```

瓶颈在 Backend Bound

#### 二级分析

```shell
$ toplev -l2 --core S0-C0 -- ./benchmark
Backend_Bound.Memory_Bound: 52%
Backend_Bound.Core_Bound: 13%
```

主要瓶颈是 Memory Bound

#### 三级分析

```shell
$ toplev -l3 --core S0-C0 -- ./benchmark
Memory_Bound.L3_Bound: 38%
Memory_Bound.DRAM_Bound: 14%
```

L3 缓存未命中是主因

#### 代码定位

```shell
$ perf record -e MEM_LOAD_RETIRED.L3_MISS ./benchmark
$ perf report
```

发现函数  foo()  中某数组访问模式导致跳步访问（stride access），破坏缓存局部性

优化建议：
- 重构数据结构（如结构体数组 → 数组结构体）
- 使用缓存友好的访问模式
- 启用软件预取（ prefetcht0 ）

### Intel VTune Profiler

性能测试中，程序运行环境应与生产环境保持一致，待分析程序通常开启编译优化选项 `-O2/3`，同时开启调试信息选项 `-g`

#### Performance Snapshot(ps)

首先根据性能快照分析概览，程序运行时间可用作后续优化后的对比

![analysis type](./assets/analysis_type.png)

#### Hotspots(hs)

热点分析处定位最耗时的函数，并定位到具体耗时语句；

同时查看 CPU 使用率

![top hotspots](./assets/top_hotspots.png)

![Effective CPU Utilization Histogram](./assets/effective_cpu_utilization_histogram.png)

![Performance Navigator](./assets/performance_navigator.png)

#### Microarchitecture Exploration(ue)

微架构分析即对应 TMA 中各指标

![Summary](./assets/ue_01_sub_metrics.png)

