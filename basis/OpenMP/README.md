# OpenMP 程序优化示例代码

本目录包含 OpenMP 并行编程的各种示例代码，按照优化技术分为以下几类：

## 目录结构

```
OpenMP/
├── Basics/                    # OpenMP 编程简介
│   ├── serial_example.c              # 串行程序示例
│   ├── first_openmp_program.c        # 第一个 OpenMP 程序
│   ├── matrix_multiply_serial.c      # 矩阵乘法 - 串行版本
│   ├── matrix_multiply_parallel.c    # 矩阵乘法 - 并行版本
│   ├── parallel_for_basics.c         # parallel for 基本用法
│   └── omp_timing.c                  # OpenMP 计时函数
├── ParallelRegion/            # 并行区重构
│   ├── parallel_region_expansion.c   # 并行区扩张
│   ├── parallel_region_merging.c     # 并行区合并
│   ├── parallel_synchronization.c    # 同步原语
│   └── parallel_for_in_function.c    # 函数中的并行循环
├── FalseSharing/              # 避免伪共享
│   ├── false_sharing_analysis.c      # 伪共享分析
│   ├── false_sharing_padding.c       # 数据填充避免伪共享
│   └── false_sharing_reduction.c     # reduction 避免伪共享
├── Vectorization/             # 向量化指导命令
│   ├── simd_directive.c              # simd 指导语句
│   └── for_simd_directive.c          # for simd 指导语句
└── LoadBalancing/             # 负载均衡优化
    ├── collapse_directive.c          # 循环嵌套合并调度
    ├── schedule_static.c             # static 调度策略
    ├── schedule_dynamic.c            # dynamic 调度策略
    ├── schedule_guided.c             # guided 调度策略
    ├── if_clause.c                   # 条件并行
    ├── nowait_clause.c               # 消除隐式屏障
    ├── data_dependency.c             # 数据依赖处理
    └── loop_fusion.c                 # 循环合并
```

## 1. OpenMP 编程简介

### 1.1 OpenMP 是什么
- **原理**：OpenMP 是一种共享内存并行编程模型
- **特点**：基于编译指导语句，易于使用
- **示例**：`serial_example.c`, `first_openmp_program.c`

### 1.2 OpenMP 指导语句
- **原理**：使用 #pragma omp 指令指导编译器并行化
- **类型**：parallel, for, sections, single, master 等
- **示例**：`parallel_for_basics.c`

### 1.3 OpenMP 版矩阵乘
- **原理**：将矩阵乘法的循环并行化
- **方法**：使用 parallel for 指令
- **示例**：`matrix_multiply_serial.c`, `matrix_multiply_parallel.c`

## 2. 并行区重构

### 2.1 并行区扩张
- **原理**：将多个独立的并行区合并为一个
- **优点**：减少线程创建和销毁开销
- **示例**：`parallel_region_expansion.c`

### 2.2 并行区合并
- **原理**：将多个连续的并行区合并
- **方法**：使用 omp parallel 包裹多个 omp for
- **示例**：`parallel_region_merging.c`

## 3. 避免伪共享

### 3.1 分析伪共享
- **原理**：多个线程访问同一缓存行中的不同变量
- **影响**：缓存行频繁传输，降低性能
- **示例**：`false_sharing_analysis.c`

### 3.2 数据填充避免伪共享
- **原理**：在数组元素之间添加填充
- **方法**：确保不同线程的数据在不同缓存行
- **示例**：`false_sharing_padding.c`

### 3.3 数据私有避免伪共享
- **原理**：使用 reduction 子句
- **方法**：每个线程有私有副本，最后归约
- **示例**：`false_sharing_reduction.c`

## 4. 向量化指导命令

### 4.1 simd 指导语句
- **原理**：提示编译器将循环向量化
- **方法**：使用 #pragma omp simd
- **示例**：`simd_directive.c`

### 4.2 for simd 指导语句
- **原理**：结合工作共享和向量化
- **方法**：使用 #pragma omp for simd
- **示例**：`for_simd_directive.c`

## 5. 负载均衡优化

### 5.1 循环嵌套合并调度
- **原理**：将多层循环合并为一个迭代空间
- **方法**：使用 collapse(N) 子句
- **示例**：`collapse_directive.c`

### 5.2 线程调度配置策略
- **原理**：控制循环迭代的分配方式
- **类型**：
  - static：静态分配，适用于均匀计算
  - dynamic：动态分配，适用于不均匀计算
  - guided：自适应分配，结合两者优点
- **示例**：`schedule_static.c`, `schedule_dynamic.c`, `schedule_guided.c`

## 编译说明

所有示例代码使用 C 语言编写，编译指令如下：

```bash
# 基本编译（启用 OpenMP）
gcc -O2 -fopenmp -o <输出文件名> <源文件名.c>

# 启用 SIMD 指令集
gcc -O2 -fopenmp -msse4.2 -o <输出文件名> <源文件名.c>
gcc -O2 -fopenmp -mavx -o <输出文件名> <源文件名.c>
gcc -O2 -fopenmp -mavx512f -o <输出文件名> <源文件名.c>

# 运行
./<输出文件名>
```

## 环境变量

OpenMP 提供了一些环境变量来控制程序行为：

```bash
# 设置线程数
export OMP_NUM_THREADS=4

# 设置动态线程调整
export OMP_DYNAMIC=true

# 设置嵌套并行
export OMP_NESTED=true

# 设置调度策略
export OMP_SCHEDULE="static,10"
```

## 性能测试建议

1. 使用 `-O2` 或 `-O3` 编译选项开启优化
2. 使用 `omp_get_wtime()` 测量程序运行时间
3. 多次运行取平均值，避免偶然因素影响
4. 测试不同线程数的性能变化
5. 在目标硬件上测试，不同架构可能有不同表现

## 常见问题

### 1. 线程数设置
- 默认线程数通常等于 CPU 核心数
- 可以使用 `num_threads(N)` 子句或 `OMP_NUM_THREADS` 环境变量设置

### 2. 数据竞争
- 确保共享数据的访问是线程安全的
- 使用 `critical`, `atomic`, `reduction` 等子句保护共享数据

### 3. 负载不均衡
- 选择合适的调度策略（static/dynamic/guided）
- 使用 `collapse` 增加迭代次数
- 调整块大小（chunk size）

### 4. 伪共享
- 使用 `reduction` 子句
- 使用数据填充（padding）
- 使用 `aligned` 属性确保数据对齐

## 参考资料

- [OpenMP 官方网站](https://www.openmp.org/)
- [OpenMP 规范](https://www.openmp.org/specifications/)
- [OpenMP 教程](https://www.openmp.org/resources/tutorials-quick-guides/)
- [Intel OpenMP 文档](https://www.intel.com/content/www/us/en/developer/tools/oneapi/dpc-compiler-openmp-support.html)
