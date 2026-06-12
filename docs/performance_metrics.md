# 性能度量指标

- [性能度量指标](#性能度量指标)
  - [1. 执行时间](#1-执行时间)
  - [2. 计算效率](#2-计算效率)
  - [3. 访存效率](#3-访存效率)
  - [4. 吞吐量与延迟](#4-吞吐量与延迟)
  - [5. 加速比](#5-加速比)
  - [6. Amdahl 定律：重点优化热点](#6-amdahl-定律重点优化热点)
  - [7. Gustafson 定律：扩展问题规模](#7-gustafson-定律扩展问题规模)
  - [附录：Roofline 模型](#附录roofline-模型)

## 1. 执行时间

- bash 命令获取

    ```shell
    $ time ./a.out
    real    0m0.003s
    user    0m0.001s
    sys     0m0.001s
    ```

    - **real (真实时间)**：从命令开始执行到结束所经历的墙上时钟时间（Wall Clock Time）。包括 CPU 执行时间、等待 I/O 时间、进程调度等待时间等。如果程序在等待磁盘 I/O 或网络响应，real 时间会明显大于 user + sys 时间。

    - **user (用户态时间)**：进程在用户空间执行代码所消耗的 CPU 时间。即程序本身逻辑运算（如计算、循环、函数调用等）所花费的时间，不包括内核代为执行的时间。

    - **sys (内核态时间)**：进程在内核空间执行系统调用所消耗的 CPU 时间。当程序调用如 `read()`、`write()`、`mmap()` 等系统调用时，CPU 需要切换到内核态来完成操作，这部分时间计入 sys。

    **分析要点：**

    - `real ≈ user + sys`：程序是 CPU 密集型，几乎没有等待
    - `real >> user + sys`：程序存在大量 I/O 等待或被其他进程抢占 CPU
    - `sys` 占比高：程序频繁进行系统调用（大量文件/网络 I/O）
    - `user` 占比高：程序主要在进行计算

    **性能分析主要关注 `user` 时间**：`user` 反映的是程序自身计算逻辑的 CPU 消耗，是优化代码算法和计算效率的直接目标。`sys` 时间通常由系统调用决定，优化空间在于减少调用次数（如批量 I/O）；`real` 时间受系统负载影响，不稳定，不适合用于精确的性能对比。

- 代码嵌入统计

    在代码中嵌入计时指令，多次执行取平均以获取代码段耗时。x86 平台使用 `rdtsc` 指令读取 CPU 时间戳计数器（Time Stamp Counter）。

    ```c++
    #include <iostream>

    // 读取 CPU 时间戳计数器 (TSC)
    unsigned long long rdtsc() {
        unsigned hi, lo;
        asm volatile("rdtsc" : "=a"(lo), "=d"(hi));
        return ((unsigned long long)lo) | (((unsigned long long)hi) << 32);
    }

    int main() {
        const int N = 10;          // 重复执行次数
        unsigned long long cycles[N];
        unsigned long long total = 0;

        for (int j = 0; j < N; j++) {
            unsigned long long start = rdtsc();
            fun(16);               // 待测函数
            unsigned long long end = rdtsc();
            cycles[j] = end - start;
            total += cycles[j];
        }

        std::cout << "平均周期数: " << total / N << std::endl;
        return 0;
    }
    ```

    **注意事项：**
    - `rdtsc` 测量的是 CPU 周期数，换算为时间需除以 CPU 主频（如 3 GHz → 1 周期 ≈ 0.33 ns）
    - 现代 CPU 可能因乱序执行导致计时不准，可配合 `cpuid` 或 `lfence` 指令做序列化屏障
    - 多次执行取平均可减少调度抖动等噪声影响

- 工具辅助分析

    - **Perf**：Linux 自带的性能分析工具，基于内核 perf_events 子系统，支持硬件计数器采样、调用图分析等
    - **Intel VTune Profiler**：Intel 官方性能分析工具，支持热点分析、内存访问分析、线程并发分析等，可直观展示 Roofline 图

> 计算效率 与 访存效率，是 Roofline 分析中的两个重要指标

## 2. 计算效率

$$计算效率 = \frac{实测浮点性能 (GFLOPS)}{理论浮点峰值性能 (GFLOPS)} \times 100\%$$

反映程序对硬件算力的利用程度。计算效率低说明存在指令流水线停顿、分支预测失败、向量化不充分等问题。优化方向：SIMD 向量化、循环展开、减少分支。

## 3. 访存效率

$$访存效率 = \frac{有效访存带宽 (GB/s)}{理论访存带宽 (GB/s)} \times 100\%$$

反映程序对内存带宽的利用程度。访存效率低说明存在缓存命中率差、数据访问模式不连续、预取不充分等问题。优化方向：提高数据局部性、优化访存模式、减少 cache miss。

## 4. 吞吐量与延迟

- **延迟 (Latency)**：完成单次操作所需的时间。可以是一次函数调用、一次数据库查询、一次网络请求，也可以是从点击链接到页面加载完成的端到端耗时。
- **吞吐量 (Throughput)**：单位时间内完成的操作数量。如 QPS（每秒查询数）、TPS（每秒事务数）。

两者关系并非简单的此消彼长：
- 批处理系统可通过流水线并行实现**高吞吐 + 高延迟**
- 单请求优化可实现**低延迟**，但吞吐量未必高

## 5. 加速比

$$speedup = \frac{优化前执行时间}{优化后执行时间}$$

加速比衡量优化效果。speedup > 1 表示有提升，理想情况下并行化 N 个核心可获得接近 N 倍的加速比，但受限于串行部分和并行开销，实际加速比通常低于理论值。详见下方 Amdahl 定律。

## 6. Amdahl 定律：重点优化热点

$$S = \frac{1}{(1-p) + \frac{p}{S_p}}$$

其中：
- $S$: 理论加速比
- $p$: 可并行化部分占总执行时间的比例
- $S_p$: 并行部分的加速比

**推导：**

设总执行时间为 $T_{total}$，串行部分占比 $(1-p)$，并行部分占比 $p$。对并行部分施加加速比 $S_p$ 后：

$$T_{new} = (1-p) \cdot T_{total} + \frac{p \cdot T_{total}}{S_p} = T_{total} \left[ (1-p) + \frac{p}{S_p} \right]$$

因此 $S = \frac{T_{total}}{T_{new}} = \frac{1}{(1-p) + \frac{p}{S_p}}$

**关键结论：** 串行部分 $(1-p)$ 是硬上限。当 $S_p \to \infty$ 时，$S_{max} = \frac{1}{1-p}$。例如串行部分占 10% 时，最大加速比仅为 10 倍。

**实践指导：** 优化重点应放在占比最大的热点代码上，而非并行化冷门路径。

## 7. Gustafson 定律：扩展问题规模

$$S(n) = (1 - \alpha) + \alpha \cdot n$$

其中：
- $S(n)$: 扩展加速比
- $n$: 处理器数量
- $\alpha$: 可并行化部分占总工作量的比例

**推导：**

与 Amdahl 定律固定问题规模不同，Gustafson 定律假设**增加处理器时问题规模也随之增大**。使用 $n$ 个处理器，总工作量扩展为 $W' = W \cdot n$，则：

$$S(n) = \frac{(1-\alpha) \cdot W' + \frac{\alpha \cdot W'}{n}}{(1-\alpha) \cdot W'} = (1-\alpha) + \alpha \cdot n$$

**与 Amdahl 定律的区别：**

| 维度     | Amdahl 定律        | Gustafson 定律       |
| -------- | ------------------ | -------------------- |
| 问题规模 | 固定               | 随处理器扩展         |
| 关注点   | 固定问题的加速比   | 扩展问题的处理能力   |
| 串行部分 | 硬上限             | 可被"稀释"           |
| 适用场景 | 延迟敏感型任务     | 吞吐量敏感型任务     |

**实践总结：**
- Amdahl 定律：串行部分是硬瓶颈，应重点优化热点耗时
- Gustafson 定律：增加并行能力可扩大计算规模，适合科学计算、大数据等场景
- 两者视角互补，实际中通常先用 Amdahl 定律定位瓶颈，再用 Gustafson 定律评估扩展潜力

## 附录：Roofline 模型

Roofline 模型是一种直观的性能分析工具，用于判断程序是计算密集型还是访存密集型，并预测性能瓶颈。

**核心概念：**

- **算力峰值 (Peak Performance)**：硬件理论最大计算能力，单位通常为 GFLOPS
- **访存带宽 (Memory Bandwidth)**：硬件理论最大内存带宽，单位通常为 GB/s
- **算术强度 (Arithmetic Intensity)**：程序中浮点运算次数与内存访问字节数之比，单位为 FLOPS/Byte

**模型公式：**

$$
Achievable Performance = \min(Peak Performance, Arithmetic Intensity \times Memory Bandwidth)
$$

**图形表示：**

```
Performance
(GFLOPS)
    ^
    |         _______________  ← 计算上限 (Peak Performance)
    |        /
    |       /
    |      /
    |     /
    |    /  ← 斜率 = 内存带宽
    |   /
    |  /
    | /
    |/________________________> Arithmetic Intensity (FLOPS/Byte)
```

**分析方法：**

- **计算瓶颈区 (Compute Bound)**：算术强度高于拐点时，性能受限于计算能力
- **访存瓶颈区 (Memory Bound)**：算术强度低于拐点时，性能受限于内存带宽
- **拐点 (Ridge Point)**：计算上限与访存上限的交点，算术强度 = Peak Performance / Memory Bandwidth

**优化策略：**

- 处于**访存瓶颈区**：优化数据访问模式，提高缓存命中率，减少内存访问
- 处于**计算瓶颈区**：优化计算算法，使用 SIMD 向量化，提高计算效率
