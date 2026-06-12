// SSE 向量加法性能比较 (SSE Vector Addition Performance)
//
// 本例比较 float 和 int 两种数据类型使用 SSE 指令进行向量加法的性能差异。
//
// SSE 指令说明：
//   float 向量加法：
//     _mm_loadu_ps    — 加载 4 个 float（128 位）
//     _mm_add_ps      — 4 个 float 并行加法
//     _mm_store_ps    — 存储 4 个 float
//
//   int 向量加法：
//     _mm_loadu_si128 — 加载 128 位整数数据
//     _mm_add_epi32   — 4 个 int 并行加法
//     _mm_store_si128 — 存储 128 位整数数据
//
// 编译命令：
//   g++ -O2 -msse2 -Wall -o add_2 add_2.cpp

#include <immintrin.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// SSE float 向量加法（每次处理 4 个 float）
static void add_float(const float* in1, const float* in2, float* out1, const size_t n) {
    for (size_t i = 0; i < n; i += 4) {
        __m128 a      = _mm_loadu_ps(&in1[i]);
        __m128 b      = _mm_loadu_ps(&in2[i]);
        __m128 result = _mm_add_ps(a, b);
        _mm_store_ps(&out1[i], result);
    }
}

// SSE int 向量加法（每次处理 4 个 int）
static void add_int(const int* in3, const int* in4, int* out2, const size_t n) {
    __m128i x, y, z;
    for (size_t i = 0; i < n; i += 4) {
        x = _mm_loadu_si128((__m128i*)&in3[i]);
        y = _mm_loadu_si128((__m128i*)&in4[i]);
        z = _mm_add_epi32(x, y);
        _mm_store_si128((__m128i*)&out2[i], z);
    }
}

int main() {
    int n = 128;

    // 分配内存
    float* in1  = (float*)malloc(n * sizeof(float));
    float* in2  = (float*)malloc(n * sizeof(float));
    float* out1 = (float*)malloc(n * sizeof(float));
    int*   in3  = (int*)malloc(n * sizeof(int));
    int*   in4  = (int*)malloc(n * sizeof(int));
    int*   out2 = (int*)malloc(n * sizeof(int));

    // 初始化数据
    for (int i = 0; i < n; i++) {
        in1[i] = rand() % 10;
        in2[i] = rand() % 10;
        in3[i] = rand() % 10;
        in4[i] = rand() % 10;
    }

    // 测试 float 向量加法
    clock_t start = clock();
    add_float(in1, in2, out1, n);
    clock_t end = clock();
    double time_float = (double)(end - start);
    printf("SSE float  add: %.0f us\n", time_float);

    // 测试 int 向量加法
    start = clock();
    add_int(in3, in4, out2, n);
    end = clock();
    double time_int = (double)(end - start);
    printf("SSE int    add: %.0f us\n", time_int);

    printf("Speed-up (float/int): %.2fx\n", time_float / time_int);

    // 释放内存
    free(in1);
    free(in2);
    free(out1);
    free(in3);
    free(in4);
    free(out2);

    return 0;
}
