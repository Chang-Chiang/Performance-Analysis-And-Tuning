// 不同数据类型向量加
// 存储空间更小的数据类型, 运算速度更快

// g++ -O2 -msse2 -Wall -g -o add add.cpp

#include <stdio.h>
#include <emmintrin.h>
#include <sys/time.h>
#include <stdlib.h>
#include <time.h>

#define M 2000000
#define N 1000000

int main()
{
    double time_use = 0;
    struct timeval start;
    struct timeval end;
    srand(time(NULL));
    
    //short (heap-allocated)
    short *op1 = (short*)malloc(sizeof(short) * M);
    short *op2 = (short*)malloc(sizeof(short) * M);
    short *result1 = (short*)malloc(sizeof(short) * M);
    if (!op1 || !op2 || !result1) { perror("malloc"); return 1; }
    for (int i = 0; i < M; i++)
    {
        op1[i] = rand() % 10;
        op2[i] = rand() % 10;
    }

    //int (heap-allocated)
    int *op3 = (int*)malloc(sizeof(int) * N);
    int *op4 = (int*)malloc(sizeof(int) * N);
    int *result2 = (int*)malloc(sizeof(int) * N);
    if (!op3 || !op4 || !result2) { perror("malloc"); return 1; }
    for (int i = 0; i < N; i++)
    {
        op3[i] = rand() % 10;
        op4[i] = rand() % 10;
    }

    //用SSE指令进行一次short数据加法运算
    __m128i x1, y1, z1;
    gettimeofday(&start, NULL);
    for (int i = 0; i < M; i += 8)
    {
        x1 = _mm_loadu_si128((__m128i*)&op1[i]);
        y1 = _mm_loadu_si128((__m128i*)&op2[i]);
        z1 = _mm_add_epi16(x1, y1);
        _mm_storeu_si128((__m128i*)&result1[i], z1);
    }
    gettimeofday(&end, NULL);
    time_use = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
    printf("short数据向量加耗费时间: %lfus\n", time_use);

    //用SSE指令进行一次int数据加法运算
    __m128i x, y, z;
    gettimeofday(&start, NULL);
    for (int i = 0; i < N; i += 4)
    {
        x = _mm_loadu_si128((__m128i*)&op3[i]);
        y = _mm_loadu_si128((__m128i*)&op4[i]);
        z = _mm_add_epi32(x, y);
        _mm_storeu_si128((__m128i*)&result2[i], z);
    }
    gettimeofday(&end, NULL);
    time_use=(double)(end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
    printf("int数据向量加耗费时间:%lfus\n",time_use);

    free(op1);
    free(op2);
    free(result1);
    free(op3);
    free(op4);
    free(result2);

    return 0;
}