# 访存优化示例代码

本目录包含访存优化的各种示例代码，按照优化层次分为以下几类：

## 目录结构

```
Memory/
├── RegisterOptimization/      # 寄存器优化
│   ├── register_allocation.c           # 寄存器分配
│   ├── scalar_replacement_loop.c       # 标量替换 - 循环
│   ├── scalar_replacement_accumulation.c # 标量替换 - 数组累加
│   ├── scalar_replacement_computation.c # 标量替换 - 数组计算
│   ├── loop_tiling_register_reuse.c    # 循环分块 - 寄存器重用
│   └── loop_unroll_register_reuse.c    # 循环展开 - 寄存器重用
├── CacheOptimization/         # 缓存优化
│   ├── false_sharing.c                 # 伪共享问题
│   ├── false_sharing_solution.c        # 伪共享解决方案
│   ├── data_prefetch.c                 # 数据预取
│   ├── help_thread_prefetch.c          # 帮助线程预取
│   ├── matrix_multiply.c               # 矩阵乘法 - 优化前
│   └── matrix_multiply_cache_blocking.c # 矩阵乘法 - 缓存分块
├── MemoryOptimization/        # 内存优化
│   ├── reduce_memory_access.c          # 减少内存读写
│   ├── data_alignment.c                # 数据对齐
│   ├── array_access_patterns.c         # 数组访问模式
│   ├── memory_access_optimization.c    # 内存访问优化 - 缓存行对齐
│   └── indirect_memory_access.c        # 间接内存访问
├── DiskOptimization/          # 磁盘优化
│   └── multi_threaded_matrix_sum.c     # 多线程矩阵求和
└── DataLayout/                # 数据布局
    ├── array_restructuring.c           # 数组重组
    ├── array_transposition.c           # 数组转置
    ├── struct_field_adjustment.c       # 结构属性域调整
    ├── struct_splitting.c              # 结构体拆分
    └── struct_array_conversion.c       # 结构体数组转换
```

## 1. 寄存器优化

### 1.1 寄存器分配
- **原理**：将频繁访问的变量分配到寄存器中，减少内存访问
- **方法**：使用局部变量、register 关键字
- **示例**：`register_allocation.c`

### 1.2 寄存器重用
- **原理**：通过标量替换，将数组元素保存到寄存器中重复使用
- **方法**：引入临时变量、循环分块、循环展开
- **示例**：
  - `scalar_replacement_loop.c` - 循环中的标量替换
  - `scalar_replacement_accumulation.c` - 数组累加的标量替换
  - `scalar_replacement_computation.c` - 数组计算的标量替换
  - `loop_tiling_register_reuse.c` - 循环分块提高寄存器重用
  - `loop_unroll_register_reuse.c` - 循环展开提高寄存器重用

## 2. 缓存优化

### 2.1 减少伪共享
- **原理**：多个线程访问同一缓存行中的不同变量，导致缓存行频繁传输
- **方法**：使用局部变量、填充对齐
- **示例**：
  - `false_sharing.c` - 伪共享问题演示
  - `false_sharing_solution.c` - 伪共享解决方案

### 2.2 数据预取
- **原理**：提前将数据从内存加载到缓存，减少缓存未命中延迟
- **方法**：使用 `__builtin_prefetch` 内置函数、帮助线程预取
- **示例**：
  - `data_prefetch.c` - 编译器内置预取函数
  - `help_thread_prefetch.c` - 帮助线程预取

### 2.3 缓存分块
- **原理**：将大矩阵分解为小块，使每个小块能放入缓存
- **方法**：循环分块、调整循环顺序
- **示例**：
  - `matrix_multiply.c` - 传统矩阵乘法
  - `matrix_multiply_cache_blocking.c` - 缓存分块矩阵乘法

## 3. 内存优化

### 3.1 减少内存读写
- **原理**：通过保存中间结果到寄存器，减少内存访问次数
- **方法**：使用临时变量、减少数据依赖
- **示例**：`reduce_memory_access.c`

### 3.2 数据对齐
- **原理**：数据对齐到特定边界，提高内存访问效率
- **方法**：调整结构体成员顺序、使用对齐属性
- **示例**：`data_alignment.c`

### 3.3 直接内存访问
- **原理**：使用连续内存访问模式，提高缓存命中率
- **方法**：顺序访问、避免间接访问
- **示例**：
  - `array_access_patterns.c` - 数组访问模式
  - `memory_access_optimization.c` - 缓存行对齐
  - `indirect_memory_access.c` - 间接访问优化

## 4. 磁盘优化

### 4.1 多线程操作
- **原理**：利用多核处理器并行处理数据
- **方法**：任务分割、负载均衡
- **示例**：`multi_threaded_matrix_sum.c`

## 5. 数据布局

### 5.1 数组重组
- **原理**：将多个独立数组合并为结构体数组，提高数据局部性
- **方法**：AoS（Array of Structures）布局
- **示例**：`array_restructuring.c`

### 5.2 数组转置
- **原理**：改变数组存储顺序，使访问模式更符合缓存预取策略
- **方法**：矩阵转置、循环交换
- **示例**：`array_transposition.c`

### 5.3 结构属性域调整
- **原理**：调整结构体成员顺序，使相关数据连续存放
- **方法**：按访问模式分组成员
- **示例**：`struct_field_adjustment.c`

### 5.4 结构体拆分
- **原理**：将大结构体拆分为小结构体，减少不必要的数据加载
- **方法**：按访问频率拆分成员
- **示例**：`struct_splitting.c`

### 5.5 结构体数组转换
- **原理**：将 AoS 转换为 SoA，提高数据局部性和向量化效率
- **方法**：AoS → SoA 转换
- **示例**：`struct_array_conversion.c`

## 编译说明

所有示例代码使用 C 语言编写，编译指令如下：

```bash
# 基本编译
gcc -O2 -o <输出文件名> <源文件名.c>

# 需要 OpenMP 支持的代码
gcc -O2 -fopenmp -o <输出文件名> <源文件名.c>

# 需要 pthread 库的代码
gcc -O2 -lpthread -o <输出文件名> <源文件名.c>
```

## 运行说明

```bash
./<输出文件名>
```

## 性能测试建议

1. 使用 `-O2` 或 `-O3` 编译选项开启优化
2. 使用 `time` 命令测量程序运行时间
3. 使用性能分析工具（如 perf, gprof）分析热点
4. 多次运行取平均值，避免偶然因素影响
5. 在目标硬件上测试，不同架构可能有不同表现

## 参考资料

- [Intel® 64 and IA-32 Architectures Optimization Reference Manual](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html)
- [GCC Documentation: Built-in Functions](https://gcc.gnu.org/onlinedocs/gcc/Other-Builtins.html)
- [Computer Architecture: A Quantitative Approach](https://www.elsevier.com/books/computer-architecture/a-quantitative-approach/9780128119051)
