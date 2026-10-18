/**
 * 矩阵乘法 - 串行版本（作为并行加速比的基线）
 *
 * 原理：
 * 采用 i-k-j 循环顺序（沿 C、A、B 均按行连续访问，缓存友好），
 * 与并行版使用完全相同的算法与数据布局，唯一区别是不并行化外层循环。
 * 用于给 matrix_multiply_parallel 提供 Amdahl 加速比的分母。
 *
 * 编译指令：
 * g++ -O2 -std=c++17 -march=native matrix_multiply_serial.cpp -o matrix_multiply_serial
 *
 * 运行：
 * ./matrix_multiply_serial
 */

#include <cstdio>
#include <vector>
#include <chrono>

#ifndef N
#define N 1200
#endif

int main() {
    std::vector<float> A(size_t(N) * N, 0.f), B(size_t(N) * N, 0.f), C(size_t(N) * N, 0.f);

    // 初始化：A 行含 i+1，B 全 1 → C[i][j] = (i+1)*N，便于校验
    for (int i = 0; i < N; ++i)
        for (int j = 0; j < N; ++j) {
            A[size_t(i) * N + j] = i + 1.0f;
            B[size_t(i) * N + j] = 1.0f;
        }

    auto t0 = std::chrono::high_resolution_clock::now();

    // i-k-j 顺序：内层 j 连续写 C[i][j]、连续读 B[k][j]，复用性好
    for (int i = 0; i < N; ++i)
        for (int k = 0; k < N; ++k) {
            float a = A[size_t(i) * N + k];
            for (int j = 0; j < N; ++j)
                C[size_t(i) * N + j] += a * B[size_t(k) * N + j];
        }

    auto t1 = std::chrono::high_resolution_clock::now();
    double sec = std::chrono::duration<double>(t1 - t0).count();

    double sum = 0.0;
    for (size_t idx = 0; idx < size_t(N) * N; ++idx) sum += C[idx];
    printf("串行版  N=%d  sum=%.0f  time=%.4f s\n", N, sum, sec);
    return 0;
}
