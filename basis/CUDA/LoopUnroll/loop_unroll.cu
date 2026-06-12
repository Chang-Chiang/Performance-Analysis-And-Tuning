/**
 * 循环展开示例
 *
 * 原理：
 * 循环展开（Loop Unrolling）是一种通过减少循环控制开销
 * 来提高程序性能的技术。
 *
 * 在 CUDA 中，循环展开可以：
 * 1. 减少循环计数器更新和条件判断的开销
 * 2. 提高指令级并行性
 * 3. 更好地利用 GPU 的流水线
 *
 * 编译指令：
 * nvcc -O2 -arch=sm_35 -o loop_unroll loop_unroll.cu
 *
 * 运行：
 * ./loop_unroll
 */

#include <stdio.h>
#include <cuda_runtime.h>
#include "common.h"

/**
 * 基础核函数：未展开循环
 * 每次循环迭代处理一个元素
 */
__global__ void MatrixMulKernel_Basic(float* Md, float* Nd, float* Pd, int width)
{
    extern __shared__ float data[];

    const int tid = threadIdx.x;
    const int row = blockIdx.x;
    int i, j;

    // 将一行数据加载到共享内存
    for (i = tid; i < width; i += blockDim.x)
    {
        data[i] = Md[row * width + i];
    }
    __syncthreads();

    // 使用共享内存进行计算
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
 * 循环展开核函数：展开 2 次
 * 每次循环迭代处理两个元素
 */
__global__ void MatrixMulKernel_Unroll2(float* Md, float* Nd, float* Pd, int width)
{
    extern __shared__ float data[];

    const int tid = threadIdx.x;
    const int row = blockIdx.x;
    int i, j;

    // 将一行数据加载到共享内存
    for (i = tid; i < width; i += blockDim.x)
    {
        data[i] = Md[row * width + i];
    }
    __syncthreads();

    // 使用共享内存进行计算（循环展开 2 次）
    double tmp = 0.0;
    for (j = tid; j < width; j += blockDim.x)
    {
        tmp = 0.0;
        // 循环展开：每次迭代处理 2 个元素
        for (i = 0; i < width / 2; i++)
        {
            tmp += data[i] * Nd[i * width + j];
            tmp += data[i + 1] * Nd[(i + 1) * width + j];
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
 * 主机端矩阵乘法（循环展开版本）
 */
void MatrixMulOnHost_Unroll2(float *A, float *B, float *C, int width)
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

    /* 矩阵乘法（循环展开 2 次） */
    for (i = 0; i < width; i++)
    {
        for (j = 0; j < width; j++)
        {
            temp = 0.0;
            for (k = 0; k < width / 2; k++)
            {
                temp += A[i * width + k] * B1[j * width + k];
                temp += A[i * width + k + 1] * B1[j * width + k + 1];
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

    /* 主机端矩阵乘法（循环展开） */
    iStart = seconds();
    MatrixMulOnHost_Unroll2(M, N, P, Width);
    iElaps = seconds() - iStart;
    printf("MatrixMulOnHost_Unroll2: \t %f sec\n", iElaps);

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
    MatrixMulKernel_Basic<<<grid, block, sizeof(float) * Width>>>(Md, Nd, Pd, Width);
    CHECK(cudaDeviceSynchronize());
    iElaps = seconds() - iStart;
    printf("MatrixMulKernel_Basic: \t %f sec\n", iElaps);

    /* 测试循环展开版本 */
    iStart = seconds();
    MatrixMulKernel_Unroll2<<<grid, block, sizeof(float) * Width>>>(Md, Nd, Pd, Width);
    CHECK(cudaDeviceSynchronize());
    iElaps = seconds() - iStart;
    printf("MatrixMulKernel_Unroll2: \t %f sec\n", iElaps);

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
 * 循环展开详解：
 *
 * 1. 什么是循环展开：
 *    - 将循环体复制多次，减少循环迭代次数
 *    - 每次迭代处理多个元素
 *    - 减少循环控制开销
 *
 * 2. 循环展开的优点：
 *    - 减少循环计数器更新和条件判断的开销
 *    - 提高指令级并行性
 *    - 更好地利用 GPU 的流水线
 *    - 减少分支预测失败
 *
 * 3. 循环展开的缺点：
 *    - 增加代码体积
 *    - 可能导致寄存器溢出
 *    - 可能导致指令缓存未命中
 *
 * 4. 展开因子选择：
 *    - 通常选择 2, 4, 8 等 2 的幂次
 *    - 需要考虑目标架构的特性
 *    - 可以通过实验找到最优的展开因子
 *
 * 5. 在 CUDA 中的使用：
 *    - 使用 #pragma unroll 指令让编译器自动展开
 *    - 手动展开循环
 *    - 需要确保循环次数是展开因子的倍数
 *
 * 6. 示例：
 *    // 未展开
 *    for (int i = 0; i < n; i++)
 *        sum += data[i];
 *
 *    // 展开 2 次
 *    for (int i = 0; i < n; i += 2)
 *    {
 *        sum += data[i];
 *        sum += data[i + 1];
 *    }
 */
