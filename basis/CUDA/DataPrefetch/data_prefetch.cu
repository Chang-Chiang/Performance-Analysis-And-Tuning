/**
 * 数据预取示例
 *
 * 原理：
 * 数据预取（Data Prefetch）是一种通过提前将数据从慢速内存加载到快速内存
 * 来隐藏内存访问延迟的技术。
 *
 * 在 CUDA 中，可以使用设备内存（__device__）来实现数据预取：
 * 1. 在计算开始前，将数据从全局内存加载到设备内存
 * 2. 在计算过程中，从设备内存读取数据
 * 3. 设备内存的访问速度比全局内存快
 *
 * 编译指令：
 * nvcc -O2 -arch=sm_35 -o data_prefetch data_prefetch.cu
 *
 * 运行：
 * ./data_prefetch
 */

#include <stdio.h>
#include <cuda_runtime.h>
#include "common.h"

/**
 * 基础核函数：直接访问全局内存
 * 每次循环迭代都访问全局内存
 */
__global__ void MatrixMulKernel_Basic(float* A, float* B, float* C, int width)
{
    int tx = threadIdx.x;
    int bx = blockIdx.x;

    for (bx = blockIdx.x; bx < width; bx += gridDim.x)
    {
        for (tx = threadIdx.x; tx < width; tx += blockDim.x)
        {
            float Pvalue = 0;
            for (int k = 0; k < width; k++)
            {
                // 每次迭代都访问全局内存
                float Mdelement = A[bx * width + k];
                float Ndelement = B[k * width + tx];
                Pvalue += Mdelement * Ndelement;
            }
            C[bx * width + tx] = Pvalue;
        }
    }
}

/**
 * 数据预取核函数
 * 在计算开始前，将数据从全局内存加载到设备内存
 *
 * 优化策略：
 * 1. 使用 __device__ 声明设备内存变量
 * 2. 在计算开始前，将数据加载到设备内存
 * 3. 在计算过程中，从设备内存读取数据
 */
__global__ void MatrixMulKernel_Preload(float* A, float* B, float* C, int width)
{
    int tx = threadIdx.x;
    int bx = blockIdx.x;

    // 声明设备内存变量
    extern __device__ float data1[];
    extern __device__ float data2[];

    // 数据预取：将数据从全局内存加载到设备内存
    for (int i = tx; i < width; i += blockDim.x)
    {
        data1[i] = A[bx * width + i];
        data2[i] = B[i * width + tx];
    }

    // 使用预取的数据进行计算
    for (bx = blockIdx.x; bx < width; bx += gridDim.x)
    {
        for (tx = threadIdx.x; tx < width; tx += blockDim.x)
        {
            float Pvalue1 = 0;
            for (int k = 0; k < width; k++)
            {
                // 从设备内存读取数据，比全局内存快
                Pvalue1 += data1[k] * data2[k];
            }
            C[bx * width + tx] = Pvalue1;
        }
    }
}

/**
 * 主机端矩阵乘法
 */
void MatrixMulOnHost(float *A, float *B, float *C, int width)
{
    int i, j, k;
    double temp = 0.0;
    float *B1;

    /* 转置矩阵 B */
    B1 = (float *)malloc(sizeof(float) * width * width);
    for (i = 0; i < width; i++)
    {
        for (j = 0; j < width; j++)
        {
            B1[i * width + j] = B[j * width + i];
        }
    }

    /* 矩阵乘法 */
    for (i = 0; i < width; i++)
    {
        for (j = 0; j < width; j++)
        {
            temp = 0.0;
            for (k = 0; k < width; k++)
            {
                temp += A[i * width + k] * B1[j * width + k];
            }
            C[i * width + j] = temp;
        }
    }
    free(B1);
}

/**
 * 结果检查函数
 */
void checkResult(float *hostRef, float *gpuRef, const int N)
{
    double epsilon = 1.0E-8;
    bool match = true;

    for (int i = 0; i < N; i++)
    {
        if (abs(hostRef[i] - gpuRef[i]) > epsilon)
        {
            match = false;
            printf("host %f gpu %f\n", hostRef[i], gpuRef[i]);
            break;
        }
    }

    if (match)
        printf("Results match.\n\n");
    else
        printf("Results do not match.\n\n");
}

int main()
{
    /* 初始化设备 */
    int dev = 0;
    cudaDeviceProp deviceProp;
    CHECK(cudaGetDeviceProperties(&deviceProp, dev));
    printf("Using device %d: %s\n", dev, deviceProp.name);
    CHECK(cudaSetDevice(dev));

    /* 设置矩阵大小 */
    int Width = 1 << 11;  // 2048x2048 矩阵
    int size = Width * Width * sizeof(float);

    printf("Matrix size: %d x %d\n", Width, Width);

    /* 主机端内存分配 */
    float *M, *N, *P, *gpuRef1, *gpuRef2;
    M = (float *)malloc(size);
    N = (float *)malloc(size);
    P = (float *)malloc(size);
    gpuRef1 = (float *)malloc(size);
    gpuRef2 = (float *)malloc(size);

    /* 初始化矩阵 */
    double iStart = seconds();
    for (int i = 0; i < Width; i++)
    {
        for (int j = 0; j < Width; j++)
        {
            M[i * Width + j] = 3.0;
            N[i * Width + j] = 3.0;
        }
    }
    double iElaps = seconds() - iStart;
    printf("Initialization: \t %f sec\n", iElaps);

    /* 主机端矩阵乘法 */
    iStart = seconds();
    MatrixMulOnHost(M, N, P, Width);
    iElaps = seconds() - iStart;
    printf("MatrixMulOnHost: \t %f sec\n", iElaps);

    /* 设备端内存分配 */
    float *Md, *Nd, *Pd;
    CHECK(cudaMalloc((void**)&Md, size));
    CHECK(cudaMalloc((void**)&Nd, size));
    CHECK(cudaMalloc((void**)&Pd, size));

    /* 将数据从主机复制到设备 */
    CHECK(cudaMemcpy(Md, M, size, cudaMemcpyHostToDevice));
    CHECK(cudaMemcpy(Nd, N, size, cudaMemcpyHostToDevice));

    /* 设置执行配置 */
    dim3 block(1024);
    dim3 grid((Width + block.x - 1) / block.x);

    printf("Grid: %d, Block: %d\n", grid.x, block.x);

    /* 测试基础版本 */
    iStart = seconds();
    MatrixMulKernel_Basic<<<grid, block>>>(Md, Nd, Pd, Width);
    CHECK(cudaDeviceSynchronize());
    iElaps = seconds() - iStart;
    printf("MatrixMulKernel_Basic: \t %f sec\n", iElaps);

    /* 测试数据预取版本 */
    iStart = seconds();
    MatrixMulKernel_Preload<<<grid, block, sizeof(float) * Width>>>(Md, Nd, Pd, Width);
    CHECK(cudaDeviceSynchronize());
    iElaps = seconds() - iStart;
    printf("MatrixMulKernel_Preload: \t %f sec\n", iElaps);

    /* 释放设备内存 */
    CHECK(cudaFree(Md));
    CHECK(cudaFree(Nd));
    CHECK(cudaFree(Pd));

    /* 释放主机内存 */
    free(M);
    free(N);
    free(P);
    free(gpuRef1);
    free(gpuRef2);

    return 0;
}

/**
 * 数据预取详解：
 *
 * 1. 什么是数据预取：
 *    - 提前将数据从慢速内存加载到快速内存
 *    - 隐藏内存访问延迟
 *    - 提高计算效率
 *
 * 2. CUDA 中的内存层次：
 *    - 寄存器（Register）：最快，容量最小
 *    - 共享内存（Shared Memory）：快，容量较小
 *    - 设备内存（Device Memory）：较快，容量较大
 *    - 全局内存（Global Memory）：最慢，容量最大
 *
 * 3. 数据预取的实现：
 *    - 使用 __device__ 声明设备内存变量
 *    - 在计算开始前，将数据加载到设备内存
 *    - 在计算过程中，从设备内存读取数据
 *
 * 4. 优化效果：
 *    - 减少全局内存访问次数
 *    - 隐藏内存访问延迟
 *    - 提高计算效率
 *
 * 5. 注意事项：
 *    - 设备内存容量有限
 *    - 需要权衡预取数据量和设备内存容量
 *    - 预取操作本身也有开销
 */
