// clFFT 快速傅里叶变换示例 (clFFT Fast Fourier Transform)
//
// 什么是 clFFT？
//   clFFT 是一个基于 OpenCL 的 FFT（快速傅里叶变换）库，
//   利用 GPU 或其他 OpenCL 设备加速 FFT 计算。
//
// clFFT 的作用:
//   1. GPU 加速 — 利用 GPU 并行计算能力加速 FFT
//   2. 跨平台 — 基于 OpenCL，支持多种硬件平台
//   3. 易于使用 — 提供简洁的 C 语言接口
//
// 本例的计算流程:
//   1. 初始化 OpenCL 平台、设备、上下文、命令队列
//   2. 初始化 clFFT 库
//   3. 创建输入缓冲区并写入数据
//   4. 创建 FFT 计划（1D、单精度、复数交错格式）
//   5. 执行正向 FFT
//   6. 读取结果并释放资源
//
// 编译命令:
//   clang++ clFFT.cpp -I ./include/ -L ./lib/ -lclFFT -lOpenCL -o clFFT
//
// 选项:
//   -I ./include/  — 指定 clFFT 头文件路径
//   -L ./lib/      — 指定 clFFT 库文件路径
//   -lclFFT        — 链接 clFFT 库
//   -lOpenCL       — 链接 OpenCL 库

#include <clFFT.h>
#include <stdlib.h>

int main(void) {
    cl_int                err;
    cl_platform_id        platform = 0;
    cl_device_id          device   = 0;
    cl_context_properties props[3] = {CL_CONTEXT_PLATFORM, 0, 0};
    cl_context            ctx      = 0;
    cl_command_queue      queue    = 0;
    cl_mem                bufX;
    float*                X;
    cl_event              event = NULL;
    int                   ret   = 0;
    size_t                N     = 16;
    clfftPlanHandle       planHandle;
    clfftDim              dim          = CLFFT_1D;
    size_t                clLengths[1] = {N};

    // 初始化 OpenCL 环境
    err      = clGetPlatformIDs(1, &platform, NULL);
    err      = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
    props[1] = (cl_context_properties)platform;
    ctx      = clCreateContext(props, 1, &device, NULL, NULL, &err);
    queue    = clCreateCommandQueue(ctx, device, 0, &err);

    // 初始化 clFFT 库
    clfftSetupData fftSetup;
    err = clfftInitSetupData(&fftSetup);
    err = clfftSetup(&fftSetup);

    // 创建输入缓冲区（复数数组，实部+虚部交替存储）
    X    = (float*)malloc(N * 2 * sizeof(*X));
    bufX = clCreateBuffer(ctx, CL_MEM_READ_WRITE, N * 2 * sizeof(*X), NULL, &err);
    err  = clEnqueueWriteBuffer(queue, bufX, CL_TRUE, 0, N * 2 * sizeof(*X), X, 0, NULL, NULL);

    // 创建 FFT 计划
    err = clfftCreateDefaultPlan(&planHandle, ctx, dim, clLengths);
    err = clfftSetPlanPrecision(planHandle, CLFFT_SINGLE);           // 单精度
    err = clfftSetLayout(planHandle, CLFFT_COMPLEX_INTERLEAVED,      // 输入：复数交错
                         CLFFT_COMPLEX_INTERLEAVED);                 // 输出：复数交错
    err = clfftSetResultLocation(planHandle, CLFFT_INPLACE);         // 原地计算
    err = clfftBakePlan(planHandle, 1, &queue, NULL, NULL);          // 编译优化

    // 执行正向 FFT
    err = clfftEnqueueTransform(planHandle, CLFFT_FORWARD, 1, &queue, 0, NULL, NULL, &bufX, NULL, NULL);
    err = clFinish(queue);

    // 读取结果
    err = clEnqueueReadBuffer(queue, bufX, CL_TRUE, 0, N * 2 * sizeof(*X), X, 0, NULL, NULL);

    // 释放资源
    clReleaseMemObject(bufX);
    free(X);
    err = clfftDestroyPlan(&planHandle);
    clfftTeardown();
    clReleaseContext(ctx);

    return ret;
}
