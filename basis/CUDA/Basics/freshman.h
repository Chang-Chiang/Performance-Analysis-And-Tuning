/**
 * CUDA 初学者工具头文件
 *
 * 功能：
 * 提供 CUDA 编程中常用的工具函数，包括：
 * - 错误检查宏
 * - 计时函数
 * - 数据初始化函数
 * - 设备初始化函数
 * - 结果检查函数
 */

#ifndef FRESHMAN_H
#define FRESHMAN_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifdef _WIN32
#   include <windows.h>
#else
#   include <sys/time.h>
#endif

/**
 * CUDA 错误检查宏
 */
#define CHECK(call)                                                            \
{                                                                              \
    const cudaError_t error = call;                                            \
    if (error != cudaSuccess)                                                  \
    {                                                                          \
        printf("ERROR: %s:%d, ", __FILE__, __LINE__);                          \
        printf("code:%d, reason:%s\n", error, cudaGetErrorString(error));      \
        exit(1);                                                               \
    }                                                                          \
}

/**
 * Windows 平台的 gettimeofday 实现
 */
#ifdef _WIN32
int gettimeofday(struct timeval *tp, void *tzp)
{
    time_t clock;
    struct tm tm;
    SYSTEMTIME wtm;
    GetLocalTime(&wtm);
    tm.tm_year = wtm.wYear - 1900;
    tm.tm_mon = wtm.wMonth - 1;
    tm.tm_mday = wtm.wDay;
    tm.tm_hour = wtm.wHour;
    tm.tm_min = wtm.wMinute;
    tm.tm_sec = wtm.wSecond;
    tm.tm_isdst = -1;
    clock = mktime(&tm);
    tp->tv_sec = clock;
    tp->tv_usec = wtm.wMilliseconds * 1000;
    return (0);
}
#endif

/**
 * CPU 计时函数
 * 返回自某个固定时间点以来的秒数
 */
double cpuSecond()
{
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return ((double)tp.tv_sec + (double)tp.tv_usec * 1e-6);
}

/**
 * 初始化浮点数组
 * @param ip 数组指针
 * @param size 数组大小
 */
void initialData(float* ip, int size)
{
    time_t t;
    srand((unsigned)time(&t));
    for (int i = 0; i < size; i++)
    {
        ip[i] = (float)(rand() & 0xffff) / 1000.0f;
    }
}

/**
 * 初始化整数数组
 * @param ip 数组指针
 * @param size 数组大小
 */
void initialData_int(int* ip, int size)
{
    time_t t;
    srand((unsigned)time(&t));
    for (int i = 0; i < size; i++)
    {
        ip[i] = int(rand() & 0xff);
    }
}

/**
 * 打印矩阵
 * @param C 矩阵数据
 * @param nx 矩阵列数
 * @param ny 矩阵行数
 */
void printMatrix(float *C, const int nx, const int ny)
{
    float *ic = C;
    printf("Matrix<%d,%d>:\n", ny, nx);
    for (int i = 0; i < ny; i++)
    {
        for (int j = 0; j < nx; j++)
        {
            printf("%6.2f ", ic[j]);
        }
        ic += nx;
        printf("\n");
    }
}

/**
 * 初始化 CUDA 设备
 * @param devNum 设备编号
 */
void initDevice(int devNum)
{
    int dev = devNum;
    cudaDeviceProp deviceProp;
    CHECK(cudaGetDeviceProperties(&deviceProp, dev));
    printf("Using device %d: %s\n", dev, deviceProp.name);
    CHECK(cudaSetDevice(dev));
}

/**
 * 检查计算结果
 * @param hostRef 主机端计算结果
 * @param gpuRef GPU 计算结果
 * @param N 数组大小
 */
void checkResult(float *hostRef, float *gpuRef, const int N)
{
    double epsilon = 1.0E-8;
    for (int i = 0; i < N; i++)
    {
        if (abs(hostRef[i] - gpuRef[i]) > epsilon)
        {
            printf("Results don't match!\n");
            printf("%f(hostRef[%d]) != %f(gpuRef[%d])\n", hostRef[i], i, gpuRef[i], i);
            return;
        }
    }
    printf("Check result success!\n");
}

#endif // FRESHMAN_H
