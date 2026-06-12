# MPI 程序优化示例代码

本目录包含 MPI 并行编程的各种示例代码，按照优化技术分为以下几类：

## 目录结构

```
MPI/
├── Basics/                    # MPI 编程简介
│   ├── mympi.h                       # 工具函数头文件
│   ├── mpi_hello.c                   # MPI 入门程序
│   ├── matrix_multiply_serial.c      # 串行矩阵乘法
│   └── matrix_multiply_basic_parallel.c # 基础并行矩阵乘法
├── DataPartitioning/          # 数据划分优化
│   ├── row_partition.c               # 按行分解
│   ├── column_partition.c            # 按列分解
│   └── cannon_algorithm_blocking.c   # 棋盘式分解（阻塞式）
├── OverlapCommunication/      # 重叠通信和计算
│   └── cannon_algorithm_nonblocking.c # 棋盘式分解（非阻塞式）
├── LoadBalancing/             # 负载均衡优化
│   ├── prime_serial.c                # 串行素数筛选法
│   ├── prime_interleaved.c           # 交叉分解
│   └── prime_block.c                 # 按块分解
└── RedundantComputation/      # 冗余计算减少通信
    └── prime_redundant.c             # 冗余计算素数筛选法
```

## 1. MPI 编程简介

### 1.1 MPI 是什么
- **原理**：MPI（Message Passing Interface）是消息传递编程模型
- **特点**：进程间通过消息传递进行通信
- **示例**：`mympi.h`, `mpi_hello.c`

### 1.2 MPI 函数库
- **初始化**：MPI_Init, MPI_Finalize
- **通信**：MPI_Send, MPI_Recv, MPI_Bcast, MPI_Scatter, MPI_Gather, MPI_Reduce
- **同步**：MPI_Barrier, MPI_Wait
- **示例**：`mpi_hello.c`

### 1.3 MPI 程序编写
- **步骤**：初始化、通信、计算、同步、结束
- **编译**：mpicc -o output source.c
- **运行**：mpirun -np N ./output
- **示例**：`mpi_hello.c`

### 1.4 MPI 版矩阵乘
- **原理**：将矩阵乘法并行化
- **方法**：数据分发、并行计算、结果收集
- **示例**：`matrix_multiply_serial.c`, `matrix_multiply_basic_parallel.c`

## 2. 数据划分优化

### 2.1 按行分解
- **原理**：将矩阵 A 按行分解，每个进程计算 C 的若干行
- **方法**：MPI_Scatter 分发 A，MPI_Bcast 广播 B
- **示例**：`row_partition.c`

### 2.2 按列分解
- **原理**：将矩阵 A 按列分解，每个进程计算 C 的部分结果
- **方法**：自定义分发函数，MPI_Reduce 归约结果
- **示例**：`column_partition.c`

### 2.3 棋盘式分解
- **原理**：将矩阵分成 sqrt(P) x sqrt(P) 个子块
- **方法**：Cannon 算法，循环移位
- **示例**：`cannon_algorithm_blocking.c`

## 3. 重叠通信和计算

### 基本原理
- **原理**：使用非阻塞通信重叠通信和计算
- **方法**：MPI_Isend, MPI_Irecv, MPI_Wait
- **示例**：`cannon_algorithm_nonblocking.c`

### 代码实现
- **技术**：双缓冲、交替通信和计算
- **优势**：减少通信等待时间
- **示例**：`cannon_algorithm_nonblocking.c`

### 性能分析
- **指标**：通信时间、计算时间、加速比
- **工具**：MPI 性能分析工具
- **示例**：`cannon_algorithm_nonblocking.c`

## 4. 负载均衡优化

### 4.1 串行算法
- **原理**：埃拉托斯特尼筛法
- **复杂度**：O(N log log N)
- **示例**：`prime_serial.c`

### 4.2 交叉分解
- **原理**：每个进程处理不同位置的数
- **优点**：负载均衡
- **缺点**：缓存不友好
- **示例**：`prime_interleaved.c`

### 4.3 按块分解
- **原理**：每个进程处理连续的数
- **优点**：缓存友好
- **缺点**：负载可能不均衡
- **示例**：`prime_block.c`

## 5. 冗余计算减少通信

### 基本原理
- **原理**：通过冗余计算消除通信
- **方法**：每个进程独立计算共享数据
- **示例**：`prime_redundant.c`

### 代码实现
- **技术**：独立生成素数，无通信筛选
- **优势**：完全消除通信
- **示例**：`prime_redundant.c`

### 性能分析
- **权衡**：计算量 vs 通信量
- **适用**：通信开销较大的系统
- **示例**：`prime_redundant.c`

## 编译说明

所有示例代码使用 C 语言编写，编译指令如下：

```bash
# 基本编译（启用 MPI）
mpicc -O2 -o <输出文件名> <源文件名.c>

# 使用 MPI 实现
mpicc -O2 -lmpi -o <输出文件名> <源文件名.c>

# 运行
mpirun -np <进程数> ./<输出文件名>
```

## 运行环境

### Linux 环境
```bash
# 安装 MPI
sudo apt-get install mpich
# 或
sudo apt-get install openmpi-bin

# 编译
mpicc -O2 -o output source.c

# 运行
mpirun -np 4 ./output
```

### Windows 环境
```bash
# 安装 oneAPI
# 启动环境
"C:\Program Files (x86)\Intel\oneAPI\setvars.bat"

# 编译
mpicxx -o output source.c

# 运行
mpiexec -n 4 output
```

## 性能测试建议

1. 使用 `-O2` 或 `-O3` 编译选项开启优化
2. 使用 `MPI_Wtime()` 测量程序运行时间
3. 多次运行取平均值，避免偶然因素影响
4. 测试不同进程数的性能变化
5. 在目标硬件上测试，不同架构可能有不同表现

## 常见问题

### 1. 进程数设置
- 进程数应该根据问题规模和硬件资源设置
- 对于矩阵乘法，进程数应该能整除矩阵维度
- 对于素数筛选，进程数应该合理，避免任务量太小

### 2. 负载均衡
- 选择合适的分解策略（按行/按列/棋盘式）
- 考虑数据分布的均匀性
- 使用动态负载均衡（如果需要）

### 3. 通信优化
- 减少通信次数
- 使用非阻塞通信重叠计算
- 使用集合通信代替点对点通信

### 4. 内存管理
- 每个进程有独立的内存空间
- 注意内存分配和释放
- 避免内存泄漏

## 参考资料

- [MPI 官方文档](https://www.mpi-forum.org/docs/)
- [MPI 教程](https://mpitutorial.com/)
- [OpenMPI 文档](https://www.open-mpi.org/doc/)
- [MPICH 文档](https://www.mpich.org/documentation/)
