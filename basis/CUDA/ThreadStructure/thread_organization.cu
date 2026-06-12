/**
 * 线程组织优化示例 - Block 线程循环
 *
 * 原理：
 * 当矩阵大小超过线程总数时，需要让每个线程处理多个元素。
 * 通过在核函数中使用循环，可以让每个线程处理多个矩阵元素。
 *
 * 优化前：
 * - 每个线程只处理一个元素
 * - 当矩阵很大时，需要创建大量线程
 *
 * 优化后：
 * - 每个线程处理多个元素
 * - 减少线程数量，提高线程利用率
 *
 * 编译指令：
 * nvcc -O2 -o thread_organization thread_organization.cu
 *
 * 运行：
 * ./thread_organization
 */

#include <stdio.h>
#include <cuda_runtime.h>
#include "common.h"

/**
 * 基础核函数：每个线程处理一个元素
 * 当矩阵很大时，需要创建大量线程
 */
__global__ void MatrixMulKernel01(float* A, float* B, float* C, int width)
{
    int tx = threadIdx.x;
    int bx = blockDim.x;
    int idx = bx * width + tx;
    int row = idx / width;
    int col = idx % width;

    if (row < width && col < width)
    {
        float Pvalue = 0;
        for (int k = 0; k < width; k++)
        {
            float Mdelement = A[row * width + k];
            float Ndelement = B[k * width + col];
            Pvalue += Mdelement * Ndelement;
        }
        C[row * width + col] = Pvalue;
    }
}

/**
 * 优化核函数：Block 线程循环
 * 每个线程处理多个元素，通过循环实现
 *
 * 优化策略：
 * - 外层循环遍历 block（行方向）
 * - 内层循环遍历 thread（列方向）
 * - 每个线程计算多个结果元素
 */
__global__ void MatrixMulKernel02(float* A, float* B, float* C, int width)
{
    int tx = threadIdx.x;
    int bx = blockIdx.x;

    /* 外层循环：block 遍历行
     * bx 从 blockIdx.x 开始，每次增加 gridDim.x
     * 这样可以处理超过 gridDim.x 行的情况 */
    for (bx = blockIdx.x; bx < width; bx += gridDim.x)
    {
        /* 内层循环：thread 遍历列
         * tx 从 threadIdx.x 开始，每次增加 blockDim.x
         * 这样可以处理超过 blockDim.x 列的情况 */
        for (tx = threadIdx.x; tx < width; tx += blockDim.x)
        {
            float Pvalue = 0;
            for (int k = 0; k < width; k++)
            {
                float Mdelement = A[bx * width + k];
                float Ndelement = B[k * width + tx];
                Pvalue += Mdelement * Ndelement;
            }
            C[bx * width + tx] = Pvalue;
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

    /* 转置矩阵 B 以提高缓存命中率 */
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
    int Width = 1 << 10;  // 1024x1024 矩阵
    int size = Width * Width * sizeof(float);

    printf("Matrix size: %d x %d\n", Width, Width);

    /* 主机端内存分配 */
    float *M, *N, *P, *gpuRef01, *gpuRef02;
    M = (float *)malloc(size);
    N = (float *)malloc(size);
    P = (float *)malloc(size);
    gpuRef01 = (float *)malloc(size);
    gpuRef02 = (float *)malloc(size);

    /* 初始化矩阵 */
    double iStart = seconds();
    for (int i = 0; i < Width; i++)
    {
        for (int j = 0; j < Width; j++)
        {
            M[i * Width + j] = 2.0;
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

    printf("Grid size: %d\n", grid.x);
    printf("Block size: %d\n", block.x);

    /* 测试核函数 1：基础版本 */
    iStart = seconds();
    MatrixMulKernel01<<<grid, block>>>(Md, Nd, Pd, Width);
    CHECK(cudaDeviceSynchronize());
    iElaps = seconds() - iStart;
    printf("MatrixMulKernel01 on device <<<%d, %d>>>: \t %f sec\n",
           grid.x, block.x, iElaps);
    CHECK(cudaMemcpy(gpuRef01, Pd, size, cudaMemcpyDeviceToHost));
    checkResult(P, gpuRef01, Width * Width);

    /* 测试核函数 2：Block 线程循环优化 */
    iStart = seconds();
    MatrixMulKernel02<<<grid, block>>>(Md, Nd, Pd, Width);
    CHECK(cudaDeviceSynchronize());
    iElaps = seconds() - iStart;
    printf("MatrixMulKernel02 on device <<<%d, %d>>>: \t %f sec\n",
           grid.x, block.x, iElaps);
    CHECK(cudaMemcpy(gpuRef02, Pd, size, cudaMemcpyDeviceToHost));
    checkResult(P, gpuRef02, Width * Width);

    /* 释放设备内存 */
    CHECK(cudaFree(Md));
    CHECK(cudaFree(Nd));
    CHECK(cudaFree(Pd));

    /* 释放主机内存 */
    free(M);
    free(N);
    free(P);
    free(gpuRef01);
    free(gpuRef02);

    return 0;
}

/**
 * 线程组织优化要点：
 *
 * 1. 线程数量选择：
 *    - 线程数应该足够大以充分利用 GPU
 *    - 但不能太大，否则会增加调度开销
 *    - 通常选择 256, 512, 1024 等值
 *
 * 2. Block 大小选择：
 *    - 应该是 32 的倍数（warp 大小）
 *    - 通常选择 128, 256, 512, 1024
 *    - 考虑共享内存和寄存器使用量
 *
 * 3. Grid 大小选择：
 *    - 应该足够覆盖所有数据
 *    - 可以小于数据量，通过循环处理
 *    - 考虑 GPU 的 SM 数量
 *
 * 4. 循环优化：
 *    - 外层循环遍历 block
 *    - 内层循环遍历 thread
 *    - 减少线程数量，提高线程利用率
 */
