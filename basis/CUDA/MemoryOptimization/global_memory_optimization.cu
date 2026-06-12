/**
 * 全局内存优化示例
 *
 * 原理：
 * 全局内存（Global Memory）是 GPU 中最大但最慢的内存。
 * 优化全局内存访问可以显著提高程序性能。
 *
 * 优化策略：
 * 1. 合并访问（Coalesced Access）：确保 warp 内的线程访问连续的内存地址
 * 2. 减少访问次数：使用寄存器或共享内存缓存数据
 * 3. 使用只读缓存：使用 __ldg() 函数访问只读数据
 *
 * 编译指令：
 * nvcc -O2 -arch=sm_35 -o global_memory_optimization global_memory_optimization.cu
 *
 * 运行：
 * ./global_memory_optimization
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
 * 优化核函数：使用设备内存缓存
 * 将一行数据缓存到设备内存中，减少全局内存访问
 */
__global__ void MatrixMulKernel_DeviceMemory(float* Md, float* Nd, float* Pd, int width)
{
    // 使用设备内存缓存一行数据
    extern __device__ float data[];

    const int tid = threadIdx.x;
    const int row = blockIdx.x;
    int i, j;

    // 将一行数据加载到设备内存
    for (i = tid; i < width; i += blockDim.x)
    {
        data[i] = Md[row * width + i];
    }
    __syncthreads();

    // 使用缓存的数据进行计算
    double tmp = 0.0;
    for (j = tid; j < width; j += blockDim.x)
    {
        tmp = 0.0;
        for (i = 0; i < width; i++)
        {
            tmp += data[i] * Nd[i * width + j];
        }
        Pd[row * width + j] = tmp;
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
        printf("Arrays match.\n\n");
    else
        printf("Arrays do not match.\n\n");
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

    /* 测试设备内存优化版本 */
    iStart = seconds();
    MatrixMulKernel_DeviceMemory<<<grid, block, sizeof(float) * Width>>>(Md, Nd, Pd, Width);
    CHECK(cudaDeviceSynchronize());
    iElaps = seconds() - iStart;
    printf("MatrixMulKernel_DeviceMemory: \t %f sec\n", iElaps);

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
 * 全局内存优化要点：
 *
 * 1. 合并访问（Coalesced Access）：
 *    - 同一个 warp 内的线程应该访问连续的内存地址
 *    - 这样可以将多个访问合并为一个内存事务
 *    - 提高内存带宽利用率
 *
 * 2. 减少访问次数：
 *    - 使用寄存器缓存频繁访问的数据
 *    - 使用共享内存缓存块内共享的数据
 *    - 使用设备内存缓存一行数据
 *
 * 3. 使用只读缓存：
 *    - 使用 __ldg() 函数访问只读数据
 *    - 可以利用只读缓存（L1 cache）
 *    - 提高访问速度
 *
 * 4. 避免非对齐访问：
 *    - 确保数据地址对齐到内存事务大小
 *    - 避免跨缓存行访问
 *
 * 5. 使用纹理内存：
 *    - 对于只读数据，可以使用纹理内存
 *    - 纹理内存有专门的缓存
 *    - 适合空间局部性好的访问模式
 */
