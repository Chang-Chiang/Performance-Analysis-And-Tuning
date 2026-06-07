// ROCm BLAS 示例 (ROCm Basic Linear Algebra Subprograms)
//
// 什么是 rocBLAS？
//   rocBLAS 是 AMD GPU 上的 BLAS 库实现，基于 ROCm 平台。
//   类似于 NVIDIA 的 cuBLAS，rocBLAS 针对 AMD GPU 高度优化。
//
// rocBLAS 的作用:
//   1. GPU 加速 — 利用 AMD GPU 并行计算能力加速线性代数运算
//   2. 接口标准 — 兼容 BLAS 标准接口
//   3. 高度优化 — 针对 AMD GPU 架构优化
//
// 本例使用 rocblas_sscal 计算向量缩放:
//   x = alpha * x
//
//   rocblas_sscal(handle, n, alpha, x, incx)
//     handle — rocBLAS 句柄
//     n      — 向量长度
//     alpha  — 缩放系数
//     x      — 向量数据（设备内存）
//     incx   — 向量步长
//
// 编译命令:
//   clang++ rocmblas.cpp -lrocblas -L /opt/rocm/rocblas/lib/ -o rocmblas
//
// 选项:
//   -lrocblas                   — 链接 rocBLAS 库
//   -L /opt/rocm/rocblas/lib/   — 指定 rocBLAS 库路径

#include <iostream>
#include <vector>

#include "hip/hip_runtime_api.h"
#include "rocblas.h"

using namespace std;

int main() {
    rocblas_int   n     = 10240;
    float         alpha = 10.0;
    vector<float> hx(n);
    float*        dx;
    rocblas_handle handle;

    // 初始化 rocBLAS 句柄
    rocblas_create_handle(&handle);

    // 分配设备内存
    hipMalloc(&dx, n * sizeof(float));

    // 初始化主机数据
    srand(1);
    for (int i = 0; i < n; ++i) {
        hx[i] = rand() % 10 + 1;
    }

    // 将数据从主机复制到设备
    hipMemcpy(dx, hx.data(), sizeof(float) * n, hipMemcpyHostToDevice);

    // 执行向量缩放: x = alpha * x
    rocblas_status status = rocblas_sscal(handle, n, &alpha, dx, 1);

    // 检查执行状态
    if (status == rocblas_status_success) {
        cout << "status == rocblas_status_success" << endl;
    } else {
        cout << "rocblas failure: status = " << status << endl;
    }

    // 将结果从设备复制回主机
    hipMemcpy(hx.data(), dx, sizeof(float) * n, hipMemcpyDeviceToHost);

    // 释放资源
    hipFree(dx);
    rocblas_destroy_handle(handle);

    return 0;
}
