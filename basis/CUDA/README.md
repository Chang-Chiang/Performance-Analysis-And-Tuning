# CUDA 程序优化示例代码

本目录包含 CUDA 并行编程的各种示例代码，按照优化技术分为以下几类：

## 目录结构

```
CUDA/
├── Basics/                    # CUDA 编程简介
│   ├── common.h                      # 通用工具头文件
│   ├── freshman.h                    # 初学者工具头文件
│   ├── cuda_programming_basics.cu    # CUDA 编程基础
│   └── matrix_multiply_basic.cu      # 矩阵乘法基础
├── ThreadStructure/           # 线程结构优化
│   ├── thread_organization.cu        # 线程组织优化
│   └── thread_layout.cu              # 线程布局优化
├── BranchOptimization/        # 分支优化
│   └── reduce_divergence.cu          # 归约操作中的分支发散
├── MemoryOptimization/        # 访存优化
│   ├── global_memory_optimization.cu # 全局内存优化
│   └── shared_memory_optimization.cu # 共享内存优化
├── DataPrefetch/              # 数据预取
│   └── data_prefetch.cu              # 数据预取示例
└── LoopUnroll/                # 循环展开
    └── loop_unroll.cu                # 循环展开示例
```

## 1. CUDA 编程简介

### 1.1 CUDA 是什么
- **原理**：CUDA 是 NVIDIA 的并行计算平台和编程模型
- **特点**：利用 GPU 的大规模并行计算能力
- **示例**：`common.h`, `freshman.h`

### 1.2 CUDA 编程模型
- **原理**：Host/Device 模型，CPU 和 GPU 协同工作
- **结构**：Grid/Block/Thread 层次结构
- **示例**：`cuda_programming_basics.cu`

### 1.3 CUDA 程序编写
- **步骤**：分配内存、复制数据、启动核函数、复制结果
- **工具**：cudaMalloc, cudaMemcpy, 核函数调用
- **示例**：`cuda_programming_basics.cu`

### 1.4 CUDA 版矩阵乘
- **原理**：将矩阵乘法并行化，每个线程计算一个元素
- **方法**：使用 2D Grid 和 2D Block
- **示例**：`matrix_multiply_basic.cu`

## 2. 线程结构优化

### 2.1 线程组织优化
- **原理**：合理组织线程，提高利用率
- **方法**：Block 线程循环，减少线程数量
- **示例**：`thread_organization.cu`

### 2.2 线程布局优化
- **原理**：使用 2D 布局映射 2D 数据
- **方法**：2D Grid, 2D Block
- **示例**：`thread_layout.cu`

## 3. 分支优化

### 3.1 基本原理
- **原理**：同一 warp 内的线程必须执行相同指令
- **问题**：分支发散导致性能下降
- **示例**：`reduce_divergence.cu`

### 3.2 代码实现
- **方法**：减少分支发散，使用 predication
- **技巧**：确保同一 warp 内的线程执行相同分支
- **示例**：`reduce_divergence.cu`

### 3.3 性能分析
- **指标**：分支发散率、warp 执行效率
- **工具**：Nsight, nvprof
- **示例**：`reduce_divergence.cu`

## 4. 访存优化

### 4.1 全局内存优化
- **原理**：全局内存是最慢的内存，需要优化访问模式
- **方法**：合并访问、减少访问次数
- **示例**：`global_memory_optimization.cu`

### 4.2 共享内存优化
- **原理**：共享内存是快速的片上内存
- **方法**：将频繁访问的数据加载到共享内存
- **示例**：`shared_memory_optimization.cu`

### 4.3 避免 bank 冲突
- **原理**：共享内存分为多个 bank，冲突会降低性能
- **方法**：调整数据访问模式，避免 bank 冲突
- **示例**：`shared_memory_optimization.cu`

### 4.4 高速缓存优化
- **原理**：利用 L1/L2 缓存提高访问速度
- **方法**：数据预取、访问模式优化
- **示例**：`global_memory_optimization.cu`

## 5. 数据预取

### 5.1 基本原理
- **原理**：提前将数据从慢速内存加载到快速内存
- **目的**：隐藏内存访问延迟
- **示例**：`data_prefetch.cu`

### 5.2 代码实现
- **方法**：使用设备内存缓存数据
- **技巧**：在计算开始前加载数据
- **示例**：`data_prefetch.cu`

### 5.3 性能分析
- **指标**：内存访问延迟、计算效率
- **工具**：Nsight, nvprof
- **示例**：`data_prefetch.cu`

## 6. 循环展开

### 6.1 基本原理
- **原理**：减少循环控制开销，提高指令级并行
- **方法**：将循环体复制多次
- **示例**：`loop_unroll.cu`

### 6.2 代码实现
- **方法**：手动展开或使用 #pragma unroll
- **技巧**：选择合适的展开因子
- **示例**：`loop_unroll.cu`

### 6.3 性能分析
- **指标**：指令数、循环开销、寄存器使用
- **工具**：Nsight, nvprof
- **示例**：`loop_unroll.cu`

## 编译说明

所有示例代码使用 CUDA C++ 编写，编译指令如下：

```bash
# 基本编译
nvcc -O2 -o <输出文件名> <源文件名.cu>

# 指定计算能力
nvcc -O2 -arch=sm_35 -o <输出文件名> <源文件名.cu>
nvcc -O2 -arch=sm_60 -o <输出文件名> <源文件名.cu>
nvcc -O2 -arch=sm_70 -o <输出文件名> <源文件名.cu>

# 生成调试信息
nvcc -O2 -g -G -o <输出文件名> <源文件名.cu>

# 运行
./<输出文件名>
```

## 性能测试建议

1. 使用 `-O2` 或 `-O3` 编译选项开启优化
2. 使用 `cudaEvent` 或 `cudaDeviceSynchronize()` 测量核函数执行时间
3. 多次运行取平均值，避免偶然因素影响
4. 测试不同线程配置的性能变化
5. 在目标硬件上测试，不同架构可能有不同表现

## 常见问题

### 1. 线程配置选择
- Block 大小通常选择 128, 256, 512, 1024
- Grid 大小根据数据量和 Block 大小计算
- 考虑 GPU 的 SM 数量和最大线程数

### 2. 内存管理
- 使用 cudaMalloc/cudaFree 管理设备内存
- 使用 cudaMemcpy 在主机和设备之间复制数据
- 注意内存对齐和访问模式

### 3. 分支发散
- 确保同一 warp 内的线程执行相同分支
- 使用 predication 代替分支
- 重新组织数据访问模式

### 4. 共享内存使用
- 合理分配共享内存大小
- 使用 __syncthreads() 同步
- 避免 bank 冲突

## 参考资料

- [CUDA 官方文档](https://docs.nvidia.com/cuda/)
- [CUDA 编程指南](https://docs.nvidia.com/cuda/cuda-c-programming-guide/)
- [CUDA 最佳实践指南](https://docs.nvidia.com/cuda/cuda-c-best-practices-guide/)
- [Nsight 文档](https://developer.nvidia.com/nsight-visual-studio-edition)
