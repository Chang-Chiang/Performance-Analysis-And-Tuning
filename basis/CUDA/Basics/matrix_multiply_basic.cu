/**
 * CUDA 矩阵乘法基础示例
 *
 * 原理：
 * 展示如何在 GPU 上实现矩阵乘法，包括：
 * - 二维线程组织（2D Grid, 2D Block）
 * - 矩阵元素的索引计算
 * - 主机端和设备端的矩阵乘法对比
 *
 * 编译指令：
 * nvcc -O2 -o matrix_multiply_basic matrix_multiply_basic.cu
 *
 * 运行：
 * ./matrix_multiply_basic
 */

#include <stdio.h>
#include <cuda_runtime.h>
#include "common.h"

/**
 * 矩阵乘法核函数（2D Grid, 2D Block）
 * 每个线程计算结果矩阵的一个元素
 *
 * @param Md 输入矩阵 M
 * @param Nd 输入矩阵 N
 * @param Pd 输出矩阵 P
 * @param width 矩阵宽度（假设方阵）
 */
__global__ void MatrixMulKernel(float* Md, float* Nd, float* Pd, int width)
{
    /* 计算线程的全局坐标
     * tx: 列坐标
     * bx: 行坐标 */
    int tx = threadIdx.x + blockIdx.x * blockDim.x;
    int bx = threadIdx.y + blockIdx.y * blockDim.y;

    /* 将二维坐标转换为一维索引 */
    int idx = bx * width + tx;
    int row = idx / width;
    int col = idx % width;

    /* 边界检查 */
    if (row < width && col < width)
    {
        float Pvalue = 0;
        /* 计算矩阵乘法：P[row][col] = sum(M[row][k] * N[k][col]) */
        for (int k = 0; k < width; k++)
        {
            float Mdelement = Md[row * width + k];
            float Ndelement = Nd[k * width + col];
            Pvalue += Mdelement * Ndelement;
        }
        Pd[row * width + col] = Pvalue;
    }
}

/**
 * 主机端矩阵乘法
 * 用于验证 GPU 计算结果
 */
void MatrixMulOnHost(float *A, float *B, float *C, int width)
{
    int i, j, k;
    double temp = 0.0;

    /* 转置矩阵 B 以提高缓存命中率 */
    float *B1;
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
    /* 设置矩阵大小 */
    int Width = 1 << 10;  // 1024x1024 矩阵
    int size = Width * Width * sizeof(float);

    printf("Matrix size: %d x %d\n", Width, Width);
    printf("Memory size: %d bytes\n", size);

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

    /* 设置执行配置
     * 使用 2D Block: 32x32 = 1024 线程 */
    int dimx = 32;
    int dimy = 32;
    dim3 block(dimx, dimy);
    dim3 grid((Width + block.x - 1) / block.x, (Width + block.y - 1) / block.y);

    printf("Grid size: (%d, %d)\n", grid.x, grid.y);
    printf("Block size: (%d, %d)\n", block.x, block.y);

    /* 启动核函数 */
    iStart = seconds();
    MatrixMulKernel<<<grid, block>>>(Md, Nd, Pd, Width);
    CHECK(cudaDeviceSynchronize());
    iElaps = seconds() - iStart;
    printf("MatrixMulKernel on device <<<(%d,%d), (%d,%d)>>>: \t %f sec\n",
           grid.x, grid.y, block.x, block.y, iElaps);

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
 * 矩阵乘法优化方向：
 *
 * 1. 线程组织优化：
 *    - 使用 2D Block 提高数据局部性
 *    - 调整 Block 大小以适应硬件限制
 *
 * 2. 内存优化：
 *    - 使用共享内存减少全局内存访问
 *    - 使用常量内存存储小量只读数据
 *
 * 3. 计算优化：
 *    - 循环展开减少循环开销
 *    - 数据预取隐藏内存延迟
 *
 * 4. 分支优化：
 *    - 减少 warp 内的分支发散
 *    - 使用 predication 代替分支
 */
