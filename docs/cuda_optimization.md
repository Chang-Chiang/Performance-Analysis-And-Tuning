# CUDA 程序优化

## 1. CUDA 编程简介

### 1.1 什么是 CUDA

CUDA（Compute Unified Device Architecture，统一计算设备架构）是 NVIDIA 提出的通用并行计算平台和编程模型，为使用 GPU 的异构计算开发提供了便捷高效的开发环境。异构计算采用并行或分布式计算方式，通过协调地使用性能、结构各异的计算器件以满足不同的计算需求。由 CPU 处理器与众核 GPU 可组成一个典型的异构计算架构。

**CUDA 平台生态：**

CUDA 平台支持开发者使用 C/C++、FORTRAN、Python 等行业标准语言的扩展来构建 CUDA 程序。丰富的加速库包括：CUFFT、CUBLAS、CURAND、CUSPARSE、CULA、MAGMA、Thrust、NPP、PhysX、OptiX 等。CUDA C 是标准 ANSI C 语言的一个扩展，被广泛应用于各领域 CUDA 程序的开发。

**CUDA API 层次：**

CUDA 提供两层应用程序接口 API 来管理 GPU 设备：

- **CUDA 驱动 API**：能够细致全面地控制 GPU 设备的运行状态，但编程难度较大
- **CUDA 运行时 API**：更高级的 API，实现于驱动 API 上层，简化了 GPU 设备管理操作

**NVCC 编译流程：**

NVCC 编译器在编译过程中会将主机代码与设备代码进行分离：
- 主机端代码（C 语言编写）→ 由本地 C 编译器编译 → CPU 执行
- 设备端代码（CUDA C 编写）→ 由 NVCC 编译器编译 → GPU 执行

**典型 CUDA 程序实现流程：**

1. 获取 GPU 设备
2. 开辟 GPU 上显存空间
3. 发起主机向设备的数据传输
4. 启动核函数
5. 发起设备向主机的数据传输
6. 释放 GPU 的显存空间，重置设备

```c
cudaSetDevice(0);
cudaMalloc((void**) &d_a, sizeof(float) * n);
cudaMemcpy(d_a, a, size_t count, cudaMemcpyHostToDevice);
kernel<<<blocks, threads>>>();
cudaMemcpy(a, d_a, size_t count, cudaMemcpyDeviceToHost);
cudaFree(d_a);
cudaDeviceReset();
```

### 1.2 常用 CUDA 运行时函数

**设备管理函数：**

| 函数 | 说明 |
|------|------|
| `cudaGetDeviceCount(int* count)` | 获取当前系统中可用 GPU 设备的数量 |
| `cudaSetDevice(int device)` | 选择希望调用的 GPU 设备 |
| `cudaDeviceReset()` | 显式销毁和清理当前 GPU 设备上的所有资源 |
| `cudaDeviceSynchronize()` | 阻塞主机端进程直至 GPU 设备完成计算任务 |

**内存管理函数：**

| 函数 | 说明 |
|------|------|
| `cudaMalloc(void** devPtr, size_t size)` | 在 GPU 设备上分配线性内存 |
| `cudaMemcpy(void* dst, const void* src, size_t count, cudaMemcpyKind kind)` | 主机端与设备端数据传输 |
| `cudaMallocPitch(void** devPtr, size_t* pitch, size_t width, size_t height)` | 分配对齐的线性内存 |
| `cudaMemcpy2D(...)` | 二维数据传输 |
| `cudaFree(void* devPtr)` | 释放 GPU 内存空间 |

**核函数与错误处理：**

```c
// 设备端核函数：返回类型必须为 void
__global__ void kernel_name(argument list);

// 错误信息转换
const char* cudaGetErrorString(cudaError_t error);
```

### 1.3 CUDA 程序编写示例

以向量相加为例展示完整的 CUDA 程序编写过程。

**CPU 版本：**

```c
void sumArraysOnHost(float *A, float *B, float *C, const int N) {
    for (int idx = 0; idx < N; idx++) {
        C[idx] = A[idx] + B[idx];
    }
}
```

**GPU 核函数：**

```c
__global__ void sumArraysOnGPU(float *A, float *B, float *C, const int N) {
    int tx = threadIdx.x;
    C[tx] = A[tx] + B[tx];
}
```

**实现流程：**

1. 使用 `cudaGetDeviceProperties` 获取 GPU 设备信息
2. 完成数组 A 和 B 的初始化
3. 使用 `cudaMalloc` 开辟 GPU 内存空间 d_A、d_B、d_C
4. 使用 `cudaMemcpy` 将数组 A、B 从 CPU 传输到 GPU
5. 启动核函数 `sumArraysOnGPU` 进行数组相加
6. 使用 `cudaMemcpy` 将结果数组 C 从 GPU 传回 CPU
7. 验证正确性
8. 释放显存空间，重置设备

### 1.4 CUDA 版矩阵乘法

矩阵乘法是科学计算中的基本运算。矩阵 A 的一行与矩阵 B 的一列进行向量内积，得到矩阵 C 中的一个元素。

**CPU 版本：**

```c
void MatrixMulOnHost(float *A, float *B, float *C, int width) {
    for (int i = 0; i < width; i++) {
        for (int j = 0; j < width; j++) {
            float sum = 0.0;
            for (int k = 0; k < width; k++) {
                float a = A[i * width + k];
                float b = B[k * width + j];
                sum += a * b;
            }
            C[i * width + j] = sum;
        }
    }
}
```

**GPU 核函数（初版）：**

启用 `width * width` 个线程，每个线程负责计算结果矩阵中的一个元素：

```c
__global__ void MatrixMulKernel(float* Ad, float* Bd, float* Cd, int width) {
    int offset = threadIdx.x;
    int row = offset / width;
    int col = offset % (width - 1);
    float sum = 0;
    for (int i = 0; i < width; i++) {
        sum += Ad[row * width + i] * Bd[i * width + col];
    }
    Cd[row * width + col] = sum;
}
```

**编译与测试：**

```bash
nvcc matrixmul.cu -o matrixmul
nsys profile --stats=true ./matrixmul
```

测试环境：NVIDIA RTX 3090，CUDA 11.6。

| 函数名称 | 矩阵规模 | 线程布局 | 运行时间 |
|----------|----------|----------|----------|
| MatrixMulOnHost | 32×32 | — | 262 μs |
| MatrixMulKernel | 32×32 | (1, 1024) | 5.18 μs |

---

## 2. 线程结构优化

### 2.1 线程组织优化

在编写 CUDA 程序时，可以通过优化线程的组织方式，将负责执行计算任务的 thread 划分至多个 block 上，利用多个 SM 处理器提高任务的并行程度，从而提升 GPU 设备的利用率。

初版矩阵乘核函数仅开启了一个 block 计算全部元素，多数 SM 器件处于闲置状态。优化方法是开启多个 block，每个 block 中的线程负责计算结果矩阵的部分元素。

**多 block 版核函数：**

```c
__global__ void MatrixMulKernel_multiblock(float* Ad, float* Bd, float* Cd, int width) {
    int tx = threadIdx.x + blockIdx.x * blockDim.x;
    int row = tx / width;
    int col = tx % (width - 1);
    float sum = 0;
    for (int k = 0; k < width; k++) {
        sum += Ad[row * width + k] * Bd[k * width + col];
    }
    Cd[row * width + col] = sum;
}
```

在主机端通过修改 `grid` 变量调整线程块数目。

**测试结果（矩阵规模 32×32）：**

| 函数名称 | 线程布局 | 时间 |
|----------|----------|------|
| MatrixMulOnHost | — | 70 μs |
| Kernel_MultiBlock | (1, 1024) | 5.12 μs |
| Kernel_MultiBlock | (2, 512) | 3.90 μs |
| Kernel_MultiBlock | (4, 256) | 3.74 μs |
| Kernel_MultiBlock | (8, 128) | 3.48 μs |
| Kernel_MultiBlock | (16, 64) | 3.36 μs |
| Kernel_MultiBlock | (32, 32) | 3.65 μs |

**测试结果（矩阵规模 64×64）：**

| 函数名称 | 线程布局 | 时间 |
|----------|----------|------|
| MatrixMulOnHost | — | 639 μs |
| Kernel_MultiBlock | (4, 1024) | 7.68 μs |
| Kernel_MultiBlock | (8, 512) | 5.47 μs |
| Kernel_MultiBlock | (16, 256) | 4.73 μs |
| Kernel_MultiBlock | (32, 128) | 4.70 μs |
| Kernel_MultiBlock | (64, 64) | 5.05 μs |

**测试结果（矩阵规模 1024×1024）：**

| 函数名称 | 线程布局 | 时间 |
|----------|----------|------|
| MatrixMulOnHost | — | 5.32 s |
| Kernel_MultiBlock | (1024, 1024) | 1.86 s |
| Kernel_MultiBlock | (2048, 512) | 1.73 s |

### 2.2 线程布局优化

`blockDim` 和 `gridDim` 是描述线程块和网格维度的内置变量。使用内置的二维坐标变量（`blockIdx.x`、`blockIdx.y`、`threadIdx.x`、`threadIdx.y`、`blockDim.x`、`blockDim.y`）来建立线程至目标元素的映射，比一维索引计算更为直观高效。

**二维线程布局核函数：**

```c
__global__ void MatrixMulKernel_2DGrid2DBlock(float* Ad, float* Bd, float* Cd, int width) {
    int row = blockIdx.y * blockDim.y + threadIdx.y;
    int col = blockIdx.x * blockDim.x + threadIdx.x;
    float sum = 0;
    for (int k = 0; k < width; k++) {
        sum += Ad[row * width + k] * Bd[k * width + col];
    }
    Cd[row * width + col] = sum;
}
```

**主机端线程布局设置：**

```c
dim3 block(32, 32);
dim3 grid((Width + block.x - 1) / block.x, (Width + block.y - 1) / block.y);
```

**测试结果（矩阵规模 1024×1024）：**

| 函数名称 | 线程布局 (grid, block) | 时间 (s) |
|----------|------------------------|----------|
| MatrixMulOnHost | — | 5.32 |
| 2DGrid2DBlock | ((1, 1024), (1024, 1)) | 1.8564 |
| 2DGrid2DBlock | ((2, 512), (512, 2)) | 1.0251 |
| 2DGrid2DBlock | ((4, 256), (256, 4)) | 1.0201 |
| 2DGrid2DBlock | ((8, 128), (128, 8)) | 1.0202 |
| 2DGrid2DBlock | ((16, 64), (64, 16)) | 1.0196 |
| 2DGrid2DBlock | ((32, 32), (32, 32)) | 1.0254 |
| 2DGrid2DBlock | ((64, 16), (16, 64)) | 1.0308 |
| 2DGrid2DBlock | ((128, 8), (8, 128)) | 1.3301 |
| 2DGrid2DBlock | ((256, 4), (4, 256)) | 2.2937 |
| 2DGrid2DBlock | ((512, 2), (2, 512)) | 4.3188 |
| 2DGrid2DBlock | ((1024, 1), (1, 1024)) | 8.5023 |

> **观察**：block 维度为 (16, 64) 或 (32, 32) 时性能最优；block 维度为 (1, 1024) 或 (1024, 1) 时性能最差。合理的二维布局能显著提升性能。

---

## 3. 分支优化

### 3.1 基本原理

GPU 内硬件调度的最小并行单位是**线程束（warp）**，由 32 个线程组成。流式多处理器 SM 由一个或多个线程束组成。GPU 上没有复杂的分支预测单元，线程束内的线程以**单指令流多线程（SIMT）**方式执行，即一个线程束中的 32 个线程同时执行相同的指令。

**线程束分化（Warp Divergence）：**

若核函数执行过程中存在条件分支语句，线程束中的线程按顺序串行通过多条分支路径。当任意线程进入某条分支路径时，线程束中其余线程都处于等待状态，直到该分支执行完毕。所有分支路径都执行完后，线程束中的所有线程才会回到同一条执行路径上。

线程束分化会削弱并行性，降低线程活跃度，从而影响 CUDA 程序性能。

### 3.2 并行归约中的分支问题

并行归约通过计算把多个数据结果归约为一个最终结果。根据线程所取元素位置的不同，有**相邻配对**和**交错配对**两种方法。

#### 相邻配对法

一个线程对相邻的两个元素进行求和操作。

```c
__global__ void reduce_GPU(int *g_idata, int *g_odata, unsigned int n) {
    unsigned int tid = threadIdx.x;
    if (tid >= n) return;
    int *idata = g_idata + blockIdx.x * blockDim.x;
    for (int stride = 1; stride < blockDim.x; stride *= 2) {
        if ((tid % (2 * stride)) == 0) {    // ← 导致线程束分化
            idata[tid] += idata[tid + stride];
        }
        __syncthreads();
    }
    if (tid == 0)
        g_odata[blockIdx.x] = idata[0];
}
```

> **问题**：条件语句 `if ((tid % (2 * stride)) == 0)` 使线程束内线程进入不同分支，随着迭代次数增加，线程束分化愈发严重。

#### 交错配对法

一个线程对具有固定跨度的两个元素进行求和操作，改善了线程束分化情况。

```c
__global__ void reduceNeighboredLess(int *g_idata, int *g_odata, unsigned int n) {
    unsigned int tid = threadIdx.x;
    unsigned int idx = blockIdx.x * blockDim.x + threadIdx.x;
    int *idata = g_idata + blockIdx.x * blockDim.x;
    if (idx > n) return;
    for (int stride = 1; stride < blockDim.x; stride *= 2) {
        int index = 2 * stride * tid;
        if (index < blockDim.x) {
            idata[index] += idata[index + stride];
        }
        __syncthreads();
    }
    if (tid == 0)
        g_odata[blockIdx.x] = idata[0];
}
```

### 3.3 性能分析

**测试环境：** 输入数组规模 `int size = 1 << 24`，一维网格和一维块，线程块内线程数 1024。

**编译与运行：**

```bash
nvcc reduce.cu -o reduce
nsys profile --stats=true ./reduce
```

**运行时间对比：**

| 函数名称 | 时间 (μs) |
|----------|-----------|
| reduce_CPU | 2154.11 |
| reduce_GPU（相邻配对） | 393.23 |
| reduceNeighboredLess（交错配对） | 227.13 |

`reduceNeighboredLess` 比 `reduce_GPU` 快约 1.7 倍。

**内存加载吞吐量（Nsight Compute）：**

```bash
ncu --metrics l1tex__t_bytes_pipe_lsu_mem_global_op_ld.sum.per_second ./reduce
```

| 函数名称 | 内存吞吐量 (GB/s) |
|----------|-------------------|
| reduce_GPU | 583.35 |
| reduceNeighboredLess | 1050.01 |

**线程束指令数（Nsight Compute）：**

```bash
ncu --metrics smsp__average_inst_executed_per_warp.ratio ./reduce
```

| 函数名称 | 指令数 |
|----------|--------|
| reduce_GPU | 317.94 |
| reduceNeighboredLess | 124.19 |

> **结论**：通过分支优化（交错配对替代相邻配对），消除了大量分支判断指令，减少了因分支判断带来的时延，性能显著提升。

---

## 4. 访存优化

CUDA 的存储层次包括寄存器、共享内存、本地内存、常量内存、纹理内存、全局内存等，不同存储层次具有不同的作用域、生命周期和缓存行为。

### 4.1 全局内存优化

对全局内存的访存指令以线程束为单位，通过缓存来实现加载或存储。需要关注两个特性：

- **合并内存访问**：当一个线程束中全部 32 个线程访问一个连续的内存块时，达成合并内存访问
- **对齐内存访问**：当目标首地址为设备缓存粒度（32 字节二级缓存或 128 字节一级缓存）的整数倍时，达成对齐内存访问

> **性能提示**：全局内存访问通常需要几百个时钟周期，而计算操作只需几个时钟周期。除了合并对齐访问外，提升计算访存比、复用全局内存数据同样至关重要。

#### 示例：矩阵乘法的计算访存比优化

分析线程结构优化后的核函数 `Kernel_2DGrid2DBlock`，核心计算代码 `sum += Ad[row * width + k] * Bd[k * width + col]` 中，两次全局内存读取对应一次乘累加计算，计算指令只占计算主体的三分之一。

**优化方案：** 重新构建核函数，每个线程负责计算一个 4×4 的矩阵块，使计算访存比变为 16/8，有利于隐藏全局内存访问时延。

**测试结果（矩阵规模 1024×1024）：**

| 函数名称 | 线程布局 | 时间 |
|----------|----------|------|
| MatrixMul_2DGrid2DBlock | ((16, 64), (64, 16)) | 1019.6 μs |
| MatrixMul_4x4 | ((16, 16), (16, 16)) | 455.45 μs |
| MatrixMul_4x4 | ((8, 8), (32, 32)) | 417.06 μs |

### 4.2 共享内存优化

共享内存是 GPU 上的关键内存部件，与全局内存相比具有更高的带宽和更低的延迟，其作用类似于一个可编程管理的缓存。SM 上执行的线程块中的所有线程共享该部分内存空间。

**特点：**
- 具有与线程块相同的生命周期
- 地址空间被线程块中所有线程共享
- 常用作线程块内线程通信的通道
- 过度使用会限制 SM 上活跃线程块的数量

**声明方式：**

```c
__shared__ float tile[size_y][size_x];
```

- 在核函数内声明：作用域仅为核函数内
- 在所有核函数外声明：作用域为 CUDA 程序全局

#### 示例：使用共享内存优化矩阵乘法

选择 `grid(8,8) block(32,32)` 的线程布局，通过共享内存减少全局内存访问时延：

```c
// 在核函数内静态开辟共享内存空间
__shared__ float ldsa[1024];
__shared__ float ldsb[1024];

// 线程块内 1024 个线程将矩阵 A 和 B 中的 1024 个元素
// 从全局内存转移至共享内存
// 乘累加运算时从共享内存获取目标元素，减少全局内存访问时延
```

**测试结果（矩阵规模 1024×1024）：**

| 函数名称 | 线程布局 | 时间 (μs) |
|----------|----------|-----------|
| MatrixMul_4x4 | ((8, 8), (32, 32)) | 417.06 |
| MatrixMul_Shared | ((8, 8), (32, 32)) | 255.67 |

#### 示例：共享内存优化并行归约

```c
__global__ void reduce_shared(int *g_idata, int *g_odata) {
    __shared__ int s_data[1024];
    int tid = blockIdx.x * blockDim.x + threadIdx.x;
    int tx = threadIdx.x;
    s_data[tx] = g_idata[tid];
    __syncthreads();
    for (int stride = 1; stride < blockDim.x; stride *= 2) {
        int index = 2 * stride * tx;
        if (index < blockDim.x) {
            s_data[index] += s_data[index + stride];
        }
        __syncthreads();
    }
    if (tx == 0)
        g_odata[blockIdx.x] = s_data[0];
}
```

### 4.3 避免 Bank 冲突

GPU 上的共享内存被分为 32 个大小相等的存储模块（bank），可被一个线程束内的 32 个线程同时访问。在费米架构上，连续的 4 字节数据被分配到连续的 32 个 bank 中。

**Bank 冲突：** 当一个线程束中的不同线程访问同一个 bank 中的不同字地址时，就会发生 bank 冲突，导致访问串行化。

**三种共享内存访问模式：**

| 模式 | 说明 |
|------|------|
| 无冲突访问 | 线程束中每个线程访问不同的 bank |
| 冲突访问 | 多个线程访问同一 bank 的不同地址 |
| 不规则访问 | 随机访问模式，可能产生冲突 |

#### 示例：消除归约中的 Bank 冲突

分析共享内存归约核函数，`s_data[index] += s_data[index + stride]` 语句在读取时会导致 bank 冲突。当 stride 为 1 时产生两路冲突，随 stride 增长冲突更严重。

**优化方法：** 重新构建累加操作的执行方式以避免 bank 冲突。

**测试结果：**

| 函数名称 | 时间 (μs) |
|----------|-----------|
| reduce_GPU | 393.23 |
| reduce_NeighboredLess | 227.14 |
| reduce_shared | 247.32 |
| reduce_nobankconflict | 217.11 |

**共享内存效率（Nsight Compute）：**

```bash
ncu --metrics smsp__sass_average_data_bytes_per_wavefront_mem_shared.pct ./bankconflict
```

| 函数名称 | 共享内存效率 | 共享内存读事务数 | 共享内存写事务数 |
|----------|-------------|-----------------|-----------------|
| reduce_shared | 21.11% | 3,137,536 | 1,896,866 |
| reduce_nobankconflict | 90.74% | 598,016 | 625,972 |

### 4.4 高速缓存优化

GPU 上有 4 种缓存：一级缓存、二级缓存、只读常量缓存、只读纹理缓存。每 SM 中有一个只读常量缓存和只读纹理缓存。与 CPU 不同，GPU 上只有内存加载操作会被缓存，存储操作不会被缓存。

#### 示例：矩阵转置与缓存利用

```c
// transpose1：按行合并读 A，非合并写 B（写操作不被缓存）
// transpose2：按列非合并读 A（经由缓存），合并写 B
```

**测试结果：**

| 函数名称 | 时间 (μs) |
|----------|-----------|
| transpose1 | 41.34 |
| transpose2 | 19.55 |

> `transpose2` 性能更优，因为非合并读操作经由高速缓存优化，而非合并写操作不能被缓存。

#### L1 缓存与共享内存资源分配

一级缓存和共享内存共享 SM 上的内存资源，可通过 `cudaFuncSetCacheConfig` API 动态分配资源占比：

```c
cudaError_t cudaFuncSetCacheConfig(const void* func, enum cudaFuncCache cacheConfig);
```

| 配置策略 | 说明 |
|----------|------|
| `cudaFuncCachePreferNone` | 无偏好（默认） |
| `cudaFuncCachePreferShared` | 优先 48KB 共享内存 + 16KB L1 缓存 |
| `cudaFuncCachePreferL1` | 优先 48KB L1 缓存 + 16KB 共享内存 |
| `cudaFuncCachePreferEqual` | L1 缓存和共享内存各 32KB |

---

## 5. 数据预取

### 5.1 基本原理

数据预取是指在执行第 k 次计算时，同时读取第 k+1 次迭代的数据，在计算和内存读取之间形成时间重叠，从而提升程序性能。

**未预取时的指令执行：**

```
取数 K → 执行 K → 取数 K+1 → 执行 K+1 → 取数 K+2 → 执行 K+2
```

**预取后的指令执行：**

```
取数 K → [执行 K + 取数 K+1] → [执行 K+1 + 取数 K+2] → 执行 K+2
```

> **性能背景**：GPU 上全局内存取数操作需要约 400~800 个时钟周期，而算术操作只需约 0~20 个时钟周期。数据预取可以在计算的同时预取下一次数据，掩藏读取延迟。

### 5.2 代码实现

在共享内存矩阵乘核函数的基础上添加数据预取：

- 开辟 2 块共享内存空间（数据规模从 1024 增大至 2048）
- 循环体外：将矩阵 A、B 的 1024 个元素从全局内存搬运至 `ldsa[0~1023]`、`ldsb[0~1023]`
- 循环体首次迭代：取 `ldsa[0~1023]`、`ldsb[0~1023]` 进行乘累加，同时将下一批元素搬运至 `ldsa[1024~2047]`、`ldsb[1024~2047]`
- 通过数据搬运与运算指令的交叉执行，掩藏全局内存向共享内存数据搬运的耗时

### 5.3 性能分析

**编译与运行：**

```bash
nvcc preload.cu -o preload
nsys profile --stats=true ./preload
```

**测试结果（矩阵规模 1024×1024）：**

| 函数名称 | 线程布局 | 时间 (μs) |
|----------|----------|-----------|
| MatrixMulShared_4x4 | ((8, 8), (32, 32)) | 256.58 |
| MatrixMulShared_preload | ((8, 8), (32, 32)) | 237.28 |

**测试结果（矩阵规模 512×512）：**

| 函数名称 | 线程布局 | 时间 (μs) |
|----------|----------|-----------|
| MatrixMulShared_4x4 | ((8, 8), (32, 32)) | 86.52 |
| MatrixMulShared_preload | ((8, 8), (32, 32)) | 63.10 |

> **注意**：数据预取使用了更大的共享内存空间。GPU 上共享内存存在资源限制，消耗过多存储资源会限制活跃线程块数量，需根据实际场景权衡。

---

## 6. 循环展开

### 6.1 基本原理

循环展开通过增加每次迭代计算的元素数量，减少循环迭代次数。它能消除分支和管理归纳变量，让更多的并发操作被添加到流水线上。

```c
// 原始循环
for (int i = 0; i < 100; i++) {
    a[i] = b[i] + c[i];
}

// 展开 4 次
for (int i = 0; i < 100; i += 4) {
    a[i + 0] = b[i + 0] + c[i + 0];
    a[i + 1] = b[i + 1] + c[i + 1];
    a[i + 2] = b[i + 2] + c[i + 2];
    a[i + 3] = b[i + 3] + c[i + 3];
}
```

**GPU 上的优势：**

- GPU 通过线程束间切换实现高效并发，充足的运算指令有利于提升并行性
- GPU 缺少复杂的分支预测单元，消除循环迭代中的分支判断有利于减少判断和分支预测耗时

### 6.2 代码实现

以归约核函数中最后一个 warp 的累加操作为例，原始代码：

```c
for (int stride = 16; stride > 0; stride = stride >> 1) {
    if (tx < stride)
        data[0][col] += data[0][col + stride];
    __syncthreads();
}
```

**循环展开后：**

```c
if (tx < 32) {
    data[0][col] += data[0][col + 16];
    __syncthreads();
    data[0][col] += data[0][col + 8];
    __syncthreads();
    data[0][col] += data[0][col + 4];
    __syncthreads();
    data[0][col] += data[0][col + 2];
    __syncthreads();
    data[0][col] += data[0][col + 1];
    __syncthreads();
}
__syncthreads();
if (tx == 0)
    r[blockIdx.x] = data[0][0];
```

完整的 `reduce_unroll` 核函数：

```c
__global__ void reduce_unroll(int *a, int *r) {
    __shared__ int data[32][32];
    int tid = blockIdx.x * blockDim.x + threadIdx.x;
    int tx = threadIdx.x;
    int row = tx / 32;
    int col = tx % 32;
    data[row][col] = a[tid];
    __syncthreads();

    for (int stride = 16; stride > 0; stride = stride >> 1) {
        if (row < stride)
            data[row][col] += data[row + stride][col];
        __syncthreads();
    }
}
```

### 6.3 性能分析

**编译与运行：**

```bash
nvcc reduce_unroll.cu -o reduce_unroll
nsys profile --stats=true ./reduce_unroll
```

**测试结果：**

| 函数名称 | 时间 (μs) |
|----------|-----------|
| reduce_GPU（相邻配对） | 393.23 |
| reduce_NeighboredLess（交错配对） | 227.14 |
| reduce_shared（共享内存） | 247.32 |
| reduce_nobankconflict（消除 bank 冲突） | 217.11 |
| reduce_unroll（循环展开） | 168.24 |

> **注意**：循环展开会消耗更多寄存器资源。对于迭代次数有限的循环可完全展开；对于迭代次数较多的循环，需注意避免过度展开导致寄存器溢出，反而降低性能。

---

## 总结

本章从 CUDA 编程模型、线程结构、分支优化、多层次存储结构、数据预取以及循环展开等角度介绍了 CUDA 程序优化方法。

| 优化技术 | 核心思想 | 性能收益来源 |
|----------|----------|-------------|
| 线程组织优化 | 开启多个 block 利用多个 SM | 提升 GPU 利用率 |
| 线程布局优化 | 使用二维 grid/block 布局 | 更直观高效的线程-数据映射 |
| 分支优化 | 交错配对替代相邻配对 | 消除线程束分化 |
| 全局内存优化 | 提高计算访存比 | 隐藏全局内存访问时延 |
| 共享内存优化 | 利用片上高速共享内存 | 减少全局内存访问时延 |
| 避免 Bank 冲突 | 优化共享内存访问模式 | 提升共享内存带宽利用率 |
| 高速缓存优化 | 利用缓存优化非合并访问 | 将非合并读转为缓存命中 |
| 数据预取 | 计算与取数重叠执行 | 掩藏内存读取延迟 |
| 循环展开 | 消除循环分支，增加指令级并行 | 减少分支判断开销 |
