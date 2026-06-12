/**
 * CUDA 通用工具头文件
 *
 * 功能：
 * 提供 CUDA 编程中常用的宏定义和工具函数，包括：
 * - 错误检查宏（CHECK, CHECK_CUBLAS 等）
 * - 计时函数（seconds）
 * - 设备初始化函数
 *
 * 使用方法：
 * 在 CUDA 程序中 #include "common.h" 即可使用
 */

#ifndef _COMMON_H
#define _COMMON_H

#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * CUDA 错误检查宏
 * 用于检查 CUDA API 调用的返回值
 * 如果调用失败，打印错误信息并退出程序
 */
#define CHECK(call)                                                            \
{                                                                              \
    const cudaError_t error = call;                                            \
    if (error != cudaSuccess)                                                  \
    {                                                                          \
        fprintf(stderr, "Error: %s:%d, ", __FILE__, __LINE__);                 \
        fprintf(stderr, "code: %d, reason: %s\n", error,                       \
                cudaGetErrorString(error));                                    \
        exit(1);                                                               \
    }                                                                          \
}

/**
 * cuBLAS 错误检查宏
 */
#define CHECK_CUBLAS(call)                                                     \
{                                                                              \
    cublasStatus_t err;                                                        \
    if ((err = (call)) != CUBLAS_STATUS_SUCCESS)                               \
    {                                                                          \
        fprintf(stderr, "Got CUBLAS error %d at %s:%d\n", err, __FILE__,       \
                __LINE__);                                                     \
        exit(1);                                                               \
    }                                                                          \
}

/**
 * cuRAND 错误检查宏
 */
#define CHECK_CURAND(call)                                                     \
{                                                                              \
    curandStatus_t err;                                                        \
    if ((err = (call)) != CURAND_STATUS_SUCCESS)                               \
    {                                                                          \
        fprintf(stderr, "Got CURAND error %d at %s:%d\n", err, __FILE__,       \
                __LINE__);                                                     \
        exit(1);                                                               \
    }                                                                          \
}

/**
 * cuFFT 错误检查宏
 */
#define CHECK_CUFFT(call)                                                      \
{                                                                              \
    cufftResult err;                                                           \
    if ( (err = (call)) != CUFFT_SUCCESS)                                      \
    {                                                                          \
        fprintf(stderr, "Got CUFFT error %d at %s:%d\n", err, __FILE__,        \
                __LINE__);                                                     \
        exit(1);                                                               \
    }                                                                          \
}

/**
 * cuSPARSE 错误检查宏
 */
#define CHECK_CUSPARSE(call)                                                   \
{                                                                              \
    cusparseStatus_t err;                                                      \
    if ((err = (call)) != CUSPARSE_STATUS_SUCCESS)                             \
    {                                                                          \
        fprintf(stderr, "Got error %d at %s:%d\n", err, __FILE__, __LINE__);   \
        cudaError_t cuda_err = cudaGetLastError();                             \
        if (cuda_err != cudaSuccess)                                           \
        {                                                                      \
            fprintf(stderr, "  CUDA error \"%s\" also detected\n",             \
                    cudaGetErrorString(cuda_err));                             \
        }                                                                      \
        exit(1);                                                               \
    }                                                                          \
}

/**
 * 计时函数
 * 返回自某个固定时间点以来的秒数
 * 用于测量程序执行时间
 */
inline double seconds()
{
    struct timeval tp;
    struct timezone tzp;
    int i = gettimeofday(&tp, &tzp);
    return ((double)tp.tv_sec + (double)tp.tv_usec * 1.e-6);
}

/**
 * 设备信息打印函数
 * 打印当前使用的 GPU 设备信息
 */
inline void printDeviceInfo()
{
    int dev = 0;
    cudaDeviceProp deviceProp;
    CHECK(cudaGetDeviceProperties(&deviceProp, dev));
    printf("Using device %d: %s\n", dev, deviceProp.name);
    printf("  Compute capability: %d.%d\n", deviceProp.major, deviceProp.minor);
    printf("  Total global memory: %lu bytes\n", deviceProp.totalGlobalMem);
    printf("  Shared memory per block: %lu bytes\n", deviceProp.sharedMemPerBlock);
    printf("  Max threads per block: %d\n", deviceProp.maxThreadsPerBlock);
    printf("  Max block dimensions: (%d, %d, %d)\n",
           deviceProp.maxThreadsDim[0], deviceProp.maxThreadsDim[1], deviceProp.maxThreadsDim[2]);
    printf("  Max grid dimensions: (%d, %d, %d)\n",
           deviceProp.maxGridSize[0], deviceProp.maxGridSize[1], deviceProp.maxGridSize[2]);
}

#endif // _COMMON_H

/**
 * 使用示例：
 *
 * #include "common.h"
 *
 * int main() {
 *     // 初始化设备
 *     int dev = 0;
 *     cudaDeviceProp deviceProp;
 *     CHECK(cudaGetDeviceProperties(&deviceProp, dev));
 *     CHECK(cudaSetDevice(dev));
 *
 *     // 计时
 *     double start = seconds();
 *     // ... 执行 CUDA 操作 ...
 *     double elapsed = seconds() - start;
 *     printf("Elapsed time: %f seconds\n", elapsed);
 *
 *     return 0;
 * }
 */
