/**
 * CUDA 编程基础示例
 *
 * 原理：
 * 展示 CUDA 编程的基本结构，包括：
 * - CUDA 编程模型（Host/Device）
 * - 线程组织（Grid/Block/Thread）
 * - 内存管理（cudaMalloc/cudaMemcpy/cudaFree）
 * - 核函数调用（<<<grid, block>>>）
 *
 * 编译指令：
 * nvcc -O2 -o cuda_programming_basics cuda_programming_basics.cu
 *
 * 运行：
 * ./cuda_programming_basics
 */

#include <stdio.h>
#include <cuda_runtime.h>
#include "common.h"

/**
 * 向量加法核函数
 * 每个线程计算一个元素的加法
 *
 * @param A 输入向量 A
 * @param B 输入向量 B
 * @param C 输出向量 C
 * @param N 向量长度
 */
__global__ void vectorAdd(float *A, float *B, float *C, int N)
{
    /* 计算全局线程 ID
     * blockIdx.x: 当前块在网格中的 x 坐标
     * blockDim.x: 每个块的线程数
     * threadIdx.x: 当前线程在块中的 x 坐标 */
    int idx = blockIdx.x * blockDim.x + threadIdx.x;

    /* 边界检查：确保线程 ID 不超过数组长度 */
    if (idx < N)
    {
        C[idx] = A[idx] + B[idx];
    }
}

int main()
{
    /* 设置向量大小 */
    int N = 1 << 20;  // 1M 元素
    size_t bytes = N * sizeof(float);

    printf("Vector size: %d elements\n", N);
    printf("Memory size: %lu bytes\n", bytes);

    /* 主机端内存分配 */
    float *h_A = (float*)malloc(bytes);
    float *h_B = (float*)malloc(bytes);
    float *h_C = (float*)malloc(bytes);

    /* 初始化数据 */
    for (int i = 0; i < N; i++)
    {
        h_A[i] = 1.0f;
        h_B[i] = 2.0f;
    }

    /* 设备端内存分配 */
    float *d_A, *d_B, *d_C;
    CHECK(cudaMalloc(&d_A, bytes));
    CHECK(cudaMalloc(&d_B, bytes));
    CHECK(cudaMalloc(&d_C, bytes));

    /* 将数据从主机复制到设备 */
    CHECK(cudaMemcpy(d_A, h_A, bytes, cudaMemcpyHostToDevice));
    CHECK(cudaMemcpy(d_B, h_B, bytes, cudaMemcpyHostToDevice));

    /* 设置执行配置
     * blockSize: 每个块的线程数
     * gridSize: 网格中的块数 */
    int blockSize = 256;
    int gridSize = (N + blockSize - 1) / blockSize;

    printf("Grid size: %d blocks\n", gridSize);
    printf("Block size: %d threads\n", blockSize);
    printf("Total threads: %d\n", gridSize * blockSize);

    /* 启动核函数 */
    double start = seconds();
    vectorAdd<<<gridSize, blockSize>>>(d_A, d_B, d_C, N);
    CHECK(cudaDeviceSynchronize());
    double elapsed = seconds() - start;
    printf("Kernel execution time: %f seconds\n", elapsed);

    /* 将结果从设备复制回主机 */
    CHECK(cudaMemcpy(h_C, d_C, bytes, cudaMemcpyDeviceToHost));

    /* 验证结果 */
    bool match = true;
    for (int i = 0; i < N; i++)
    {
        if (abs(h_C[i] - 3.0f) > 1e-5)
        {
            printf("Error at index %d: %f != 3.0\n", i, h_C[i]);
            match = false;
            break;
        }
    }
    if (match)
        printf("Results match!\n");

    /* 释放设备内存 */
    CHECK(cudaFree(d_A));
    CHECK(cudaFree(d_B));
    CHECK(cudaFree(d_C));

    /* 释放主机内存 */
    free(h_A);
    free(h_B);
    free(h_C);

    return 0;
}

/**
 * CUDA 编程模型：
 *
 * 1. Host（主机）：
 *    - CPU 执行的代码
 *    - 管理设备内存
 *    - 启动核函数
 *
 * 2. Device（设备）：
 *    - GPU 执行的代码
 *    - 执行核函数
 *    - 使用设备内存
 *
 * 线程组织：
 *
 * 1. Grid（网格）：
 *    - 由多个 Block 组成
 *    - 可以是一维、二维或三维
 *
 * 2. Block（线程块）：
 *    - 由多个 Thread 组成
 *    - 可以是一维、二维或三维
 *    - 同一个 Block 内的线程可以同步
 *    - 同一个 Block 内的线程可以共享内存
 *
 * 3. Thread（线程）：
 *    - 最小的执行单位
 *    - 每个线程执行相同的代码
 *    - 通过 threadIdx 区分不同的线程
 *
 * 内存管理：
 *
 * 1. cudaMalloc：分配设备内存
 * 2. cudaMemcpy：主机和设备之间复制数据
 *    - cudaMemcpyHostToDevice：主机 -> 设备
 *    - cudaMemcpyDeviceToHost：设备 -> 主机
 * 3. cudaFree：释放设备内存
 */
