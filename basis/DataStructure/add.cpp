// 不同数据类型向量加法性能比较 (Vector Addition Performance by Data Type)
//
// 本例比较 short 和 int 两种数据类型使用 SSE 指令进行向量加法的性能差异。
//
// 性能差异原因：
//   1. 数据大小 — short 占 2 字节，int 占 4 字节
//   2. SIMD 并行度 — 128 位 SSE 寄存器可同时处理 8 个 short 或 4 个 int
//   3. 内存带宽 — short 数据量更小，内存传输更快
//   4. 缓存效率 — 同样大小的缓存可容纳更多 short 数据
//
// SSE 指令说明：
//   _mm_loadu_si128    — 加载 128 位数据（未对齐）
//   _mm_add_epi16      — 8 个 short 并行加法
//   _mm_add_epi32      — 4 个 int 并行加法
//   _mm_storeu_si128   — 存储 128 位数据（未对齐）
//
// 编译命令：
//   g++ -O2 -msse2 -Wall -o add add.cpp

#include <emmintrin.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

#define M 2000000  // short 数组大小
#define N 1000000  // int 数组大小

int main() {
    double time_use = 0;
    struct timeval start, end;
    srand(time(NULL));

    // 分配 short 类型数组
    short* op1     = (short*)malloc(sizeof(short) * M);
    short* op2     = (short*)malloc(sizeof(short) * M);
    short* result1 = (short*)malloc(sizeof(short) * M);
    if (!op1 || !op2 || !result1) {
        perror("malloc");
        return 1;
    }

    // 初始化 short 数组
    for (int i = 0; i < M; i++) {
        op1[i] = rand() % 10;
        op2[i] = rand() % 10;
    }

    // 分配 int 类型数组
    int* op3     = (int*)malloc(sizeof(int) * N);
    int* op4     = (int*)malloc(sizeof(int) * N);
    int* result2 = (int*)malloc(sizeof(int) * N);
    if (!op3 || !op4 || !result2) {
        perror("malloc");
        return 1;
    }

    // 初始化 int 数组
    for (int i = 0; i < N; i++) {
        op3[i] = rand() % 10;
        op4[i] = rand() % 10;
    }

    // SSE 指令进行 short 向量加法（每次处理 8 个 short）
    __m128i x1, y1, z1;
    gettimeofday(&start, NULL);
    for (int i = 0; i < M; i += 8) {
        x1 = _mm_loadu_si128((__m128i*)&op1[i]);
        y1 = _mm_loadu_si128((__m128i*)&op2[i]);
        z1 = _mm_add_epi16(x1, y1);
        _mm_storeu_si128((__m128i*)&result1[i], z1);
    }
    gettimeofday(&end, NULL);
    time_use = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
    printf("short 向量加法 (SSE, %d elements): %.0f us\n", M, time_use);

    // SSE 指令进行 int 向量加法（每次处理 4 个 int）
    __m128i x, y, z;
    gettimeofday(&start, NULL);
    for (int i = 0; i < N; i += 4) {
        x = _mm_loadu_si128((__m128i*)&op3[i]);
        y = _mm_loadu_si128((__m128i*)&op4[i]);
        z = _mm_add_epi32(x, y);
        _mm_storeu_si128((__m128i*)&result2[i], z);
    }
    gettimeofday(&end, NULL);
    time_use = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
    printf("int   向量加法 (SSE, %d elements): %.0f us\n", N, time_use);

    // 释放内存
    free(op1);
    free(op2);
    free(result1);
    free(op3);
    free(op4);
    free(result2);

    return 0;
}
