### 2. 性能分析测量

性能分析工具分类

> 了解一下就行了吧, 重点会用 Intel Vtune Profiler 和 Perf 就好了

- **计数器类（Counter-based）**
  - 原理：利用硬件/软件性能计数器，统计特定事件发生次数
  - 特点：开销低，适合宏观分析
  - 工具：`perf stat`, `AMD uProf`, `Intel VTune Profiler`

- **采样类（Sampling-based）**
  - 原理：定时中断采样，记录程序执行位置
  - 特点：开销适中，能定位热点函数/代码行
  - 工具：`perf record`, `gprof`, `Intel VTune Profiler`

- **追踪类（Tracing-based）**
  - 原理：记录程序执行过程中的详细事件序列
  - 特点：开销较高，能分析执行流程和时序
  - 工具：`perf trace`, `ftrace`, `strace`, `ltrace`, `Intel VTune Profiler`

- **插桩类（Instrumentation-based）**
  - 原理：在代码中插入探针，收集详细运行时信息
  - 特点：开销最高，信息最详细
  - 工具：`Valgrind`, `Intel Advisor`, `Google Performance Tools`, `gprof`

- **模拟类（Simulation-based）**
  - 原理：模拟硬件行为，预测程序性能
  - 特点：无需实际运行，可分析微架构细节
  - 工具：`gem5`, `Sniper`, `SimpleScalar`

- **静态分析类（Static analysis）**
  - 原理：不运行程序，通过分析代码结构预测性能
  - 特点：零运行时开销，适合早期优化
  - 工具：编译器分析报告、`LLVM opt`、性能建模工具
