/**
 * 共享内存优化示例
 *
 * 原理：
 * 共享内存（Shared Memory）是 GPU 中速度仅次于寄存器的内存。
 * 同一个 Block 内的所有线程可以访问同一块共享内存。
 *
 * 优化策略：
 * 1. 将频繁访问的数据从全局内存加载到共享内存
 * 2. 使用 __syncthreads() 确保数据加载完成
 * 3. 在共享内存中进行计算
 *
 * 编译指令：
 * nvcc -O2 -arch=sm_35 -o shared_memory_optimization shared_memory_optimization.cu
 *
 * 运行：
 * ./shared_memory_optimization
 */

#include <stdio.h>
#include <cuda_runtime.h>
#include "common.h"

/**
 * 共享内存优化核函数
 * 将一行数据缓存到共享内存中，减少全局内存访问
 *
 * @param Md 输入矩阵 M
 * @param Nd 输入矩阵 N
 * @param Pd 输出矩阵 P
 * @param width 矩阵宽度
 */
__global__ void MatrixMulKernel_SharedMemory(float* Md, float* Nd, float* Pd, int width)
{
    // 声明共享内存
    extern __shared__ float data[];

    const int tid = threadIdx.x;
    const int row = blockIdx.x;
    int i, j;

    // 将一行数据加载到共享内存
    // 多个线程协作加载，每个线程加载多个元素
    for (i = tid; i < width; i += blockDim.x)
    {
        data[i] = Md[row * width + i];
    }

    // 同步：确保所有线程都完成了数据加载
    __syncthreads();

    // 使用共享内存中的数据进行计算
    double tmp = 0.0;
    for (j = tid; j < width; j += blockDim.x)
    {
        tmp = 0.0;
        for (i = 0; i < width; i++)
        {
            // 从共享内存读取数据，比全局内存快得多
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
    float *M, *N, *P, *gpuRef;
    M = (float *)malloc(size);
    N = (float *)malloc(size);
    P = (float *)malloc(size);
    gpuRef = (float *)malloc(size);

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

    /* 测试共享内存优化版本 */
    iStart = seconds();
    // 使用动态共享内存，大小为 sizeof(float) * Width
    MatrixMulKernel_SharedMemory<<<grid, block, sizeof(float) * Width>>>(Md, Nd, Pd, Width);
    CHECK(cudaDeviceSynchronize());
    iElaps = seconds() - iStart;
    printf("MatrixMulKernel_SharedMemory: \t %f sec\n", iElaps);

    /* 将结果从设备复制回主机 */
    CHECK(cudaMemcpy(gpuRef, Pd, size, cudaMemcpyDeviceToHost));

    /* 验证结果 */
    checkResult(P, gpuRef, Width * Width);

    /* 释放设备内存 */
    CHECK(cudaFree(Md));
    CHECK(cudaFree(Nd));
    CHECK(cudaFree(Pd));

    /* 释放主机内存 */
    free(M);
    free(N);
    free(P);
    free(gpuRef);

    return 0;
}

/**
 * 共享内存详解：
 *
 * 1. 共享内存的特点：
 *    - 位于 GPU 芯片上，速度非常快
 *    - 同一个 Block 内的所有线程可以访问
 *    - 容量有限（通常 48KB-96KB per SM）
 *    - 需要手动管理
 *
 * 2. 声明方式：
 *    - 静态声明：__shared__ float data[256];
 *    - 动态声明：extern __shared__ float data[];
 *    - 动态共享内存需要在核函数启动时指定大小
 *
 * 3. 同步：
 *    - 使用 __syncthreads() 确保所有线程都到达同步点
 *    - 必须确保所有线程都执行 __syncthreads()
 *    - 不能在分支中使用（除非所有线程都执行分支）
 *
 * 4. 优化策略：
 *    - 将频繁访问的数据加载到共享内存
 *    - 使用协作加载（多个线程共同加载）
 *    - 避免 bank 冲突
 *
 * 5. 注意事项：
 *    - 共享内存容量有限，不能加载太多数据
 *    - 需要权衡共享内存使用量和线程数量
 *    - 动态共享内存的大小在运行时确定
 */
