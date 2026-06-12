/**
 * 线程布局优化示例 - 2D Grid 和 2D Block
 *
 * 原理：
 * 使用二维线程布局可以更自然地映射二维数据结构（如矩阵），
 * 提高代码可读性和数据局部性。
 *
 * 优化前：
 * - 使用 1D Grid 和 1D Block
 * - 需要手动计算二维坐标
 *
 * 优化后：
 * - 使用 2D Grid 和 2D Block
 * - 直接使用 threadIdx 和 blockIdx 作为坐标
 *
 * 编译指令：
 * nvcc -O2 -o thread_layout thread_layout.cu
 *
 * 运行：
 * ./thread_layout
 */

#include <stdio.h>
#include <cuda_runtime.h>
#include "common.h"

/**
 * 矩阵乘法核函数（2D Grid, 2D Block）
 * 使用二维线程布局，更自然地映射矩阵结构
 *
 * @param Md 输入矩阵 M
 * @param Nd 输入矩阵 N
 * @param Pd 输出矩阵 P
 * @param width 矩阵宽度
 */
__global__ void MatrixMulKernel_2D(float* Md, float* Nd, float* Pd, int width)
{
    /* 使用二维线程坐标
     * 直接使用 threadIdx.x 和 threadIdx.y 作为列和行坐标 */
    int col = threadIdx.x + blockIdx.x * blockDim.x;
    int row = threadIdx.y + blockIdx.y * blockDim.y;

    /* 边界检查 */
    if (row < width && col < width)
    {
        float Pvalue = 0;
        /* 计算矩阵乘法 */
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
    int Width = 1 << 10;  // 1024x1024 矩阵
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
     * 使用 2D Block: 32x32 = 1024 线程
     * 使用 2D Grid: 覆盖整个矩阵 */
    int dimx = 32;
    int dimy = 32;
    dim3 block(dimx, dimy);
    dim3 grid((Width + block.x - 1) / block.x, (Width + block.y - 1) / block.y);

    printf("Grid size: (%d, %d)\n", grid.x, grid.y);
    printf("Block size: (%d, %d)\n", block.x, block.y);
    printf("Total threads: %d\n", grid.x * grid.y * block.x * block.y);

    /* 启动核函数 */
    iStart = seconds();
    MatrixMulKernel_2D<<<grid, block>>>(Md, Nd, Pd, Width);
    CHECK(cudaDeviceSynchronize());
    iElaps = seconds() - iStart;
    printf("MatrixMulKernel_2D on device <<<(%d,%d), (%d,%d)>>>: \t %f sec\n",
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
 * 线程布局优化要点：
 *
 * 1. 1D vs 2D 布局：
 *    - 1D 布局：简单，适合一维数据
 *    - 2D 布局：自然映射二维数据，代码更清晰
 *
 * 2. Block 大小选择：
 *    - 2D Block: (32, 32) = 1024 线程
 *    - 应该是 32 的倍数（warp 大小）
 *    - 考虑共享内存和寄存器使用量
 *
 * 3. Grid 大小选择：
 *    - 2D Grid: 覆盖整个矩阵
 *    - 计算公式: ((N + blockDim.x - 1) / blockDim.x, (N + blockDim.y - 1) / blockDim.y)
 *
 * 4. 性能考虑：
 *    - 2D 布局可能增加索引计算开销
 *    - 但可以提高数据局部性
 *    - 对于矩阵运算，通常 2D 布局更优
 *
 * 5. 维度限制：
 *    - Block 的 x 维度通常最大为 1024
 *    - Block 的 y 维度通常最大为 1024
 *    - 总线程数不能超过 1024
 */
