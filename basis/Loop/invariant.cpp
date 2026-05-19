// 循环不变量外提
// 在循环迭代空间内值不发生变化的变量，可以提取到循环外部，减少重复计算，提高性能

// 注意：提取至循环外的不变量需单独占用一个寄存器
// 减少了循环内可用寄存器的数量, 因此循环不变量外提后性能也可能没有提升

#include <stdio.h>
#include<stdlib.h>
#include <immintrin.h>
#define N 16

int main_1()
{
    const int M = 256;
    const int N = 256;
    float U[M], W[M], D[M];
    float dt = 5.0;

    for (int i = 1; i < N; i++)
    {
        U[i] = i;
        W[i] = i + 1;
        D[i] = i + 2;
    }

    float T1, T2;
    T1 = 1 / (dt * dt);  // 除法时间节拍数比乘法多, 外提表达式改为除法
    for (int i = 1; i < N; i++)
    {
        T2 = W[i] * W[i];
        for (int j = 1; j < M; j++)
        {
            // W[i] * W[i] 和 dt * dt 是循环不变量，可以提取到外层循环
            // U[i] = U[i] + W[i] * W[i] * D[j] / (dt * dt);
            U[i] = U[i] + T2 * D[j] * T1;
        }
    }
    printf("%f", U[1]);
}

#include<stdio.h>

// 向量化代码不变量外提
void main()
{
    float A[N], B[N];
    float C0 = 2.0;
    int i;
    __m128 v1, v2, v3;
    for (i = 0; i < N; i++)
    {
        B[i] = 1.0;
    }

    v2 = _mm_set_ps1(C0);
    
    for (i = 0; i < N; i += 4)
    {
        v1 = _mm_loadu_ps(&B[i]);
        // v2 = _mm_set_ps1(C0);  // 不变量外提
        v3 = _mm_mul_ps(v1, v2);
        _mm_store_ps(&A[i], v3);
    }

    for (i = 0; i < N; i++)
    {
        printf("%f  ", A[i]);
    }
}