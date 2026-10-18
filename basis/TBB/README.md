# TBB 并行编程示例代码

本目录包含 **Intel oneAPI Threading Building Blocks (oneTBB)** 并行编程的示例代码，
按优化技术分为若干子目录，风格与同级的 `basis/OpenMP/`、`basis/MPI/`、`basis/CUDA/` 对齐。

> **一句话定位**：OpenMP 面向"线程"（fork-join），**TBB 面向"任务"**——你只描述
> "怎么切分 + 每块干什么"，"用几个线程、怎么调度"交给运行时的**工作窃取（work-stealing）**引擎。

## 目录结构

```
TBB/
├── Basics/                          # TBB 编程简介
│   ├── tbb_hello.cpp                       # 第一个 TBB 程序（并发度 + parallel_for）
│   ├── parallel_for_basics.cpp             # parallel_for 两种写法（lambda / 经典 body）
│   ├── matrix_multiply_serial.cpp          # 矩阵乘法 - 串行基线
│   └── matrix_multiply_parallel.cpp        # 矩阵乘法 - TBB 并行版（可扫线程数）
├── ParallelAlgorithms/              # 并行算法模板
│   ├── parallel_reduce_sum.cpp             # parallel_reduce 归约（求和 / 求最值）
│   ├── parallel_scan_prefix.cpp            # parallel_scan 前缀和
│   ├── parallel_invoke_sort.cpp            # parallel_invoke 独立任务 + parallel_sort
│   └── parallel_pipeline_filter.cpp        # parallel_pipeline 三阶段流水
├── TaskGraph/                       # 动态任务与工作窃取
│   ├── task_group_fibonacci.cpp            # task_group 递归分治（含 cutoff 血泪坑）
│   └── work_stealing.cpp                   # 负载不均时 static vs auto 对比
├── LoadBalancing/                   # 分区器 / 负载均衡
│   ├── simple_partitioner.cpp              # simple（对分到 grainsize 即止）
│   ├── auto_partitioner_nested.cpp         # auto（默认）+ 嵌套并行
│   └── affinity_partitioner.cpp            # affinity（跨迭代复用块↔线程绑定）
├── FalseSharing/                    # 避免伪共享
│   ├── false_sharing_analysis.cpp          # 紧凑数组触发伪共享（对照基线）
│   ├── cache_aligned_counter.cpp           # cache_aligned_allocator 消伪共享
│   └── reduction_avoid_false_sharing.cpp   # parallel_reduce 消伪共享（首选）
├── ConcurrentContainers/            # 并发容器
│   ├── concurrent_vector.cpp               # 并发追加（无锁收集）
│   ├── concurrent_hash_map.cpp             # 并发计数（细粒度按桶锁）
│   └── concurrent_queue.cpp                # 无锁任务队列（工作池）
├── FlowGraph/                       # 流图（任意 DAG 任务依赖）
│   └── graph_pipeline.cpp                  # input_node→function_node→sink
└── MemoryAllocation/                # 可扩展内存分配器
    └── scalable_allocator.cpp              # scalable_allocator vs 默认 new
```

## 1. TBB 编程简介（`Basics/`）

- **是什么**：oneTBB 是 C++ 模板库级的共享内存并行框架，跨平台、无编译指导语句，纯库调用。
- **核心抽象**：任务（task）而非线程；`parallel_for` / `parallel_reduce` / ... 高层算法。
- **与 OpenMP 的关系**：解决同一问题（单机多核），但 TBB 的**递归对分 + 工作窃取**天然做负载均衡，
  变量作用域靠 C++ 闭包捕获（无需 `shared/private` 子句），且**支持安全的嵌套并行**。
- **示例**：`tbb_hello.cpp`, `parallel_for_basics.cpp`, `matrix_multiply_{serial,parallel}.cpp`

## 2. 并行算法模板（`ParallelAlgorithms/`）

| 算法 | 用途 | 对应 OpenMP |
|------|------|-------------|
| `parallel_for` | 数据并行循环 | `#pragma omp parallel for` |
| `parallel_reduce` | 归约（和/最值/自定义合并）| `reduction(...)` |
| `parallel_scan` | 前缀和 / 流压缩 | `#pragma omp scan` / `MPI_Scan` |
| `parallel_invoke` | 若干独立任务并行 | `#pragma omp sections` |
| `parallel_sort` | 并行排序 | （无直接对应）|
| `parallel_pipeline` | 多阶段数据流水 | （需手写）|

## 3. 动态任务与工作窃取（`TaskGraph/`）

- **task_group**：运行时随时 `g.run()` 派生子任务，`g.wait()` 汇合——适合递归分治（Fibonacci、树遍历）。
- **⚠️ 血泪坑**：递归任务**必须有串行 cutoff**，否则任务爆炸 → 栈溢出 / 段错误（见 `task_group_fibonacci.cpp`）。
- **work_stealing**：空闲线程从忙线程队列尾部"偷"未执行区间，负载自动拉平（对照 `static` 划分）。

## 4. 分区器 / 负载均衡（`LoadBalancing/`）

TBB 把 OpenMP 的 `schedule` 做成"分区器"参数：

| 分区器 | 机制 | 适用 |
|--------|------|------|
| `auto`（默认）| 递归对分 + 运行时窃取 + 支持嵌套 | 绝大多数场景首选 |
| `simple` | 对分到 grainsize 即止，开销小 | 迭代均匀、单层 |
| `static` | 恰切"线程数"块，无窃取 | 块耗时极均衡时开销最低 |
| `affinity` | 记住块↔线程绑定，跨迭代复用缓存 | 多轮迭代、数据位置稳定（时间步、迭代求解器）|

## 5. 避免伪共享（`FalseSharing/`）

第 7/8 课"多线程写同一缓存行 → 强制串行"的 TBB 版，三档阶梯：
- ❌ `false_sharing_analysis.cpp`：紧凑数组 `result[tid]`，槽位共享缓存行 → 乒乓。
- ✅ `cache_aligned_counter.cpp`：`tbb::cache_aligned_allocator` + 每槽独占一行。
- ✅✅ `reduction_avoid_false_sharing.cpp`：`parallel_reduce` 压根不用共享数组（**首选**）。

## 6. 并发容器（`ConcurrentContainers/`）

- `concurrent_vector`：多线程并发 `grow_by` 追加，无全局锁。
- `concurrent_hash_map`：按桶细粒度读写锁，适合 key 会被多线程命中的在线计数。
- `concurrent_queue`：`try_pop` 工作池模式，处理"任务数动态"的场景。
- **选型提醒**：纯离线归约用 `parallel_reduce` 往往比并发容器更快——容器用于**需在线读写/动态 key**。

## 7. 流图 / 内存分配器（`FlowGraph/`, `MemoryAllocation/`）

- **flow_graph**：用节点+边的 DAG 表达复杂依赖（分支、扇入扇出、反馈），自动流水重叠（母题①掩盖延迟）。
- **scalable_allocator**：替换默认 malloc 的全局锁瓶颈；多线程高频小分配时提升扩展性。

## 编译说明

本机 `pkg-config` 指向 **Intel oneAPI TBB**（`/opt/intel/oneapi/tbb/2023.1`）。所有示例为 C++17。

```bash
# 通用编译（自动带上头文件/库路径）
g++ -O2 -std=c++17 <文件>.cpp -o <输出> $(pkg-config --cflags --libs tbb)

# 计算密集型（如矩阵乘）建议加 -O3 -march=native
g++ -O3 -march=native -std=c++17 Basics/matrix_multiply_parallel.cpp -o par $(pkg-config --cflags --libs tbb)

# scalable_allocator 额外链接 libtbbmalloc
g++ -O2 -std=c++17 MemoryAllocation/scalable_allocator.cpp -o alloc $(pkg-config --cflags --libs tbb) -ltbbmalloc

# 若只有 apt 版（libtbb-dev）而无 oneAPI：把 $(pkg-config ...) 换成 -ltbb
```

> **oneTBB 2021+ 的 API 迁移坑**（与老 TBB 4.x 不同，本目录均已按新 API 写）：
> `parallel_reduce` 经典 body 用单个 `join()`（不再 `reverse_join`+`assign`）；
> `flow_control::stop()`（不再 `set_stop`）；`parallel_pipeline` 用 `make_filter<In,Out>(mode, body)`
> （不再 `.filter()` 链式）；`tbb::cache_line_size` 不再公有（用 64 或 `std::hardware_destructive_interference_size`）。

## 运行与控制

```bash
# 本目录多数示例接受 argv 线程数（内部用 tbb::global_control 限制最大并行度）
./matrix_multiply_parallel 8      # 上限 8 个 worker

# 默认不设限：TBB 自动用满机器并发度（本机 = 24）
./matrix_multiply_parallel
```

> oneTBB **没有**类似 `OMP_NUM_THREADS` 的统一环境变量；并行度由**运行时工作窃取调度器**
> 管理，程序内用 `tbb::global_control tbb::global_control::max_allowed_parallelism` 或
> `tbb::task_arena(N)` 显式约束（见 `matrix_multiply_parallel.cpp`）。这正是"任务≠线程"的体现。

## 本机实测（Intel Ultra 7 270K Plus，24 逻辑核 = P核+E核；oneAPI TBB 2023.1；`-O3 -march=native`，取 3 次最小值）

| 示例 | 现象 | 关键数字 |
|------|------|---------|
| 矩阵乘 N=1200 线程扫描 | 近线性→封顶 | 2→1.81x，4→3.11x，8→5.70x，16→8.98x，**24→~10x** |
| `parallel_sort` 5M int | 并行排序 | std::sort 0.254s → TBB 0.042s = **6.0x** |
| `parallel_invoke` 3 任务 | 独立任务 | 0.033s → 0.013s = **2.57x** |
| `task_group` fib(35) | 递归分治 | 0.011s → 0.002s ≈ **7x**（结果 9227465 一致）|
| `parallel_pipeline` | 三阶段流水 | 0.026s → 0.002s ≈ **10x** |
| `flow_graph` 3000 item | 流图 | ≈ **2.1~2.4x** |
| `work_stealing` | 负载不均 | auto 比 static 快 **~1.8~2.3x** |

**诚实的"未复现/无收益"项**（正好印证 `30 测量纪律手册`）：
- **伪共享**：规模仅 ~1.3ms，run1 紧凑数组慢 ~5x，run2 三者几乎持平 → 微基准太小 + 混合架构调度噪声**掩盖**效应；换更大规模/绑核才稳定。
- **`scalable_allocator`**：此负载仅 1.06x（分配不密集时看不出差别）。
- **`affinity_partitioner`**：带宽受限的 stencil 上每步耗时基本持平，本机看不出复用收益。
- **`simple` vs `auto`**：triad 访存下两者 ~0.053s 持平。

> 换机器、换负载请重测——加速比与硬件强相关。

## 参考资料

- [oneAPI Threading Building Blocks (oneTBB) 官方](https://github.com/oneapi-src/oneTBB)
- [oneTBB 在线参考手册](https://oneapi-src.github.io/oneTBB/)
- [Intel oneAPI TBB 开发者指南](https://www.intel.com/content/www/us/en/developer/tools/oneapi/onetbb.html)
