/**
 * 分支优化示例 - 归约操作中的分支发散
 *
 * 原理：
 * 在 GPU 中，同一个 warp（32 个线程）必须执行相同的指令。
 * 当 warp 内的线程执行不同的分支时，就会发生分支发散（branch divergence），
 * 导致性能下降。
 *
 * 本例展示了归约操作中的分支发散问题及其优化方法：
 * 1. 基础版本：存在分支发散
 * 2. 优化版本：减少分支发散
 *
 * 编译指令：
 * nvcc -O2 -o reduce_divergence reduce_divergence.cu
 *
 * 运行：
 * ./reduce_divergence
 */

#include <cuda_runtime.h>
#include <stdio.h>
#include "freshman.h"

/**
 * CPU 归约函数
 * 用于验证 GPU 计算结果
 */
int ArraySum_CPU(int *data, int const size)
{
    // 终止条件
    if (size == 1) return data[0];

    // 更新步长
    int const stride = size / 2;
    if (size % 2 == 1)
    {
        for (int i = 0; i < stride; i++)
        {
            data[i] += data[i + stride];
        }
        data[0] += data[size - 1];
    }
    else
    {
        for (int i = 0; i < stride; i++)
        {
            data[i] += data[i + stride];
        }
    }

    // 递归调用
    return ArraySum_CPU(data, stride);
}

/**
 * GPU 归约核函数 1：基础版本
 * 存在分支发散问题
 *
 * 问题分析：
 * - 使用 tid % (2 * stride) == 0 条件
 * - 同一个 warp 内的线程可能执行不同的分支
 * - 导致分支发散，降低性能
 */
__global__ void ArraySum_GPU01(int *g_idata, int *g_odata, unsigned int n)
{
    // 设置线程 ID
    unsigned int tid = threadIdx.x;

    // 边界检查
    if (tid >= n) return;

    // 将全局数据指针转换为当前块的局部指针
    int *idata = g_idata + blockIdx.x * blockDim.x;

    // 原地归约（in-place reduction）
    for (int stride = 1; stride < blockDim.x; stride *= 2)
    {
        // 问题：这里存在分支发散
        // 同一个 warp 内的线程可能执行不同的分支
        if ((tid % (2 * stride)) == 0)
        {
            idata[tid] += idata[tid + stride];
        }
        // 块内同步
        __syncthreads();
    }

    // 将当前块的结果写入全局内存
    if (tid == 0)
        g_odata[blockIdx.x] = idata[0];
}

/**
 * GPU 归约核函数 2：优化版本
 * 减少分支发散
 *
 * 优化策略：
 * - 使用 index = 2 * stride * tid 计算索引
 * - 确保同一个 warp 内的线程执行相同的分支
 * - 减少分支发散，提高性能
 */
__global__ void reduceNeighboredLess(int *g_idata, int *g_odata, unsigned int n)
{
    unsigned int tid = threadIdx.x;
    unsigned int idx = blockIdx.x * blockDim.x + threadIdx.x;

    // 将全局数据指针转换为当前块的局部指针
    int *idata = g_idata + blockIdx.x * blockDim.x;

    // 边界检查
    if (idx > n)
        return;

    // 原地归约（优化版本）
    for (int stride = 1; stride < blockDim.x; stride *= 2)
    {
        // 优化：使用 index = 2 * stride * tid
        // 这样同一个 warp 内的线程会执行相同的分支
        int index = 2 * stride * tid;
        if (index < blockDim.x)
        {
            idata[index] += idata[index + stride];
        }
        __syncthreads();
    }

    // 将当前块的结果写入全局内存
    if (tid == 0)
        g_odata[blockIdx.x] = idata[0];
}

int main(int argc, char** argv)
{
    // 初始化设备
    initDevice(0);

    // 设置数组大小
    int size = 1 << 23;  // 8M 元素
    printf("Array size: %d\n", size);

    // 执行配置
    int blocksize = 1024;
    if (argc > 1)
    {
        blocksize = atoi(argv[1]);
    }
    dim3 block(blocksize, 1);
    dim3 grid((size - 1) / block.x + 1, 1);
    printf("Grid: %d, Block: %d\n", grid.x, block.x);

    // 分配主机内存
    size_t bytes = size * sizeof(int);
    int *idata_host = (int*)malloc(bytes);
    int *odata_host = (int*)malloc(grid.x * sizeof(int));
    int *tmp = (int*)malloc(bytes);

    // 初始化数组
    initialData_int(idata_host, size);
    memcpy(tmp, idata_host, bytes);

    double iStart, iElaps;
    int gpu_sum = 0;

    // 分配设备内存
    int *idata_dev = NULL;
    int *odata_dev = NULL;
    CHECK(cudaMalloc((void**)&idata_dev, bytes));
    CHECK(cudaMalloc((void**)&odata_dev, grid.x * sizeof(int)));

    // CPU 归约
    int cpu_sum = 0;
    iStart = cpuSecond();
    for (int i = 0; i < size; i++)
        cpu_sum += tmp[i];
    iElaps = cpuSecond() - iStart;
    printf("CPU reduce: %lf ms, sum: %d\n", iElaps, cpu_sum);

    // GPU 归约 1：基础版本
    CHECK(cudaMemcpy(idata_dev, idata_host, bytes, cudaMemcpyHostToDevice));
    CHECK(cudaDeviceSynchronize());
    iStart = cpuSecond();
    ArraySum_GPU01<<<grid, block>>>(idata_dev, odata_dev, size);
    cudaDeviceSynchronize();
    iElaps = cpuSecond() - iStart;
    CHECK(cudaMemcpy(odata_host, odata_dev, grid.x * sizeof(int), cudaMemcpyDeviceToHost));
    gpu_sum = 0;
    for (int i = 0; i < grid.x; i++)
        gpu_sum += odata_host[i];
    printf("GPU ArraySum_GPU01: %lf ms, sum: %d\n", iElaps, gpu_sum);

    // GPU 归约 2：优化版本
    CHECK(cudaMemcpy(idata_dev, idata_host, bytes, cudaMemcpyHostToDevice));
    CHECK(cudaDeviceSynchronize());
    iStart = cpuSecond();
    reduceNeighboredLess<<<grid, block>>>(idata_dev, odata_dev, size);
    cudaDeviceSynchronize();
    iElaps = cpuSecond() - iStart;
    CHECK(cudaMemcpy(odata_host, odata_dev, grid.x * sizeof(int), cudaMemcpyDeviceToHost));
    gpu_sum = 0;
    for (int i = 0; i < grid.x; i++)
        gpu_sum += odata_host[i];
    printf("GPU reduceNeighboredLess: %lf ms, sum: %d\n", iElaps, gpu_sum);

    // 验证结果
    if (gpu_sum == cpu_sum)
    {
        printf("Test success!\n");
    }
    else
    {
        printf("Test failed! CPU: %d, GPU: %d\n", cpu_sum, gpu_sum);
    }

    // 释放内存
    free(idata_host);
    free(odata_host);
    free(tmp);
    CHECK(cudaFree(idata_dev));
    CHECK(cudaFree(odata_dev));

    // 重置设备
    cudaDeviceReset();

    return EXIT_SUCCESS;
}

/**
 * 分支发散详解：
 *
 * 1. 什么是分支发散：
 *    - GPU 中，同一个 warp（32 个线程）必须执行相同的指令
 *    - 当 warp 内的线程执行不同的分支时，就会发生分支发散
 *    - 分支发散会导致性能下降
 *
 * 2. 分支发散的影响：
 *    - 不同分支的线程必须串行执行
 *    - 假设一个 warp 中有 16 个线程执行 if 分支，16 个执行 else 分支
 *    - 那么这个 warp 的执行时间是原来的 2 倍
 *
 * 3. 如何减少分支发散：
 *    - 确保同一个 warp 内的线程执行相同的分支
 *    - 使用 predication 代替分支
 *    - 重新组织数据访问模式
 *
 * 4. 归约操作中的分支发散：
 *    - 基础版本：tid % (2 * stride) == 0
 *    - 同一个 warp 内的线程可能执行不同的分支
 *    - 优化版本：index = 2 * stride * tid
 *    - 同一个 warp 内的线程会执行相同的分支
 */
