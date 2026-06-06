# Performance-Analysis-And-Tuning
性能分析与优化

个人学习过程，总是需经过三个阶段：首先对知识整体有一个概览；在此基础上，针对问题利用已知的理论进行分析；经过对知识实际的使用，对其加深理解。
故该文以上所述思路进行梳理，在第一部分罗列参考资料中所总结的性能优化手段；第二部分，按实际项目开发过程中，性能优化过程进行展开；第三部分性能分析进行总结。

## 一. Basis 性能优化

### 1. 性能度量指标

详见 [性能度量指标](docs/performance_metrics.md)

### 2. 性能分析测量

详见 [性能分析测量](docs/performance_analysis.md)

### 3. 了解一下自己的硬件

详见 [硬件配置与调优](docs/hardware.md)

### 4. 编译器概述

详见 [编译器概述](docs/compiler.md)

### 5. 程序编写优化

详见 [程序编写优化](docs/program_optimization.md)

### 6. 单核优化

详见 [单核优化](docs/single_core_optimization.md)

### 7. 访存优化

详见 [访存优化](docs/memory_optimization.md)

### 8. OpenMP 程序优化

### 9. CUDA 程序优化

### 10. MPI 程序优化

## 二. TMA, 性能分析与优化

详见 [TMA 性能分析与优化](docs/tma.md)

## 三. 总结

> 自己做电子笔记总是力求简洁明了，但貌似也因此，逐渐缺失了言语的组织与表达能力，实际敲字的时候多少还是有点话痨。在当下快节奏的环境下，或许呈现给别人看需要精炼，但自己的思考过程及学习过程中的一些想法仍是值得记录，过程中才会涌现更多的问题，而产生问题的过程在这个 AI 时代对于个人的提升显得更为重要了。

- 进行性能分析所使用程序应与实际生产环境保持一致，开启编译优化选项 `-O2/3`，`-g` 开启调试信息则为分析所必要
- 

## 四. 性能分析优化实例

- 

## 参考

- [CPU 微架构](https://www.bilibili.com/video/BV1a2421M7Tz)
- [现代 CPU 性能分析与优化](https://github.com/dendibakh/perf-ninja)
- [](https:github.com/gongyiling/cpp_lecture)
- [程序性能优化理论与方法](https://github.com/AdvancedCompiler/AdvancedCompiler)
- [Intel® VTune™ Profiler User Guide](https://www.intel.com/content/www/us/en/docs/vtune-profiler/user-guide/2025-4/overview.html)
