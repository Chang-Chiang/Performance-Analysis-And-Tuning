// 矩阵乘

// g++ -O2 -msse3 -Wall -g -o matrix_mul matrix_mul.cpp
// g++ -O2 -march=native -Wall -g -o matrix_mul matrix_mul.cpp

#include <immintrin.h>
#include <sys/time.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>

void swap_float(float* a, float* b)
{
    float* temp = a;
    a = b;
    b = temp;
}

//float型的sse矩阵乘
void sse_mul_float(int n, float** a, float** b, float** c)
{
    __m128 t1, t2, sum;
    
    // 矩阵转置
    for (int i = 0; i < n; ++i)
    {
        for (int j = 0; j < i; ++j)
        {
            swap_float(&b[i][j], &b[j][i]);
        }
    }

    for (int i = 0; i < n; ++i)
    {
        for (int j = 0; j < n; ++j)
        {
            c[i][j] = 0.0;
            sum = _mm_setzero_ps();
            for (int k = n - 4; k >= 0; k -= 4)
            {
                t1 = _mm_loadu_ps(a[i] + k);
                t2 = _mm_loadu_ps(b[j] + k);
                t1 = _mm_mul_ps(t1, t2);
                sum = _mm_add_ps(sum, t1);
            }
            sum = _mm_hadd_ps(sum, sum);
            sum = _mm_hadd_ps(sum, sum);
            _mm_store_ss(c[i] + j, sum);
            for (int k = (n % 4) - 1; k >= 0; --k)
            {
                c[i][j] += a[i][k] * b[j][k];
            }
        }
    }

    // 矩阵转置回原样
    for (int i = 0; i < n; ++i)
    {
        for (int j = 0; j < i; ++j)
        {
            swap_float(&b[i][j], &b[j][i]);
        }
    }
}
void swap_double(double* a, double* b) {
    double* temp = a;
    a = b;
    b = temp;
}

//double型的sse矩阵乘
void sse_mul_double(int n, double** a, double** b, double** c)
{
    __m128d t1, t2, sum;

    for (int i = 0; i < n; ++i)
    {
        for (int j = 0; j < i; ++j)
        {
            swap_double(&b[i][j], &b[j][i]);
        }
    }
    for (int i = 0; i < n; ++i)
    {
        for (int j = 0; j < n; ++j)
        {
            c[i][j] = 0.0;
            sum = _mm_setzero_pd();
            for (int k = n - 2; k >= 0; k -= 2)
            {
                t1 = _mm_loadu_pd(a[i] + k);
                t2 = _mm_loadu_pd(b[j] + k);
                t1 = _mm_mul_pd(t1, t2);
                sum = _mm_add_pd(sum, t1);
            }
            sum = _mm_hadd_pd(sum, sum);
            sum = _mm_hadd_pd(sum, sum);
            _mm_store_sd(c[i] + j, sum);
            for (int k = (n % 4) - 1; k >= 0; --k)
            {
                c[i][j] += a[i][k] * b[j][k];
            }
        }
    }

    for (int i = 0; i < n; ++i)
    {
        for (int j = 0; j < i; ++j)
        {
            swap_double(&b[i][j], &b[j][i]);
        }
    }
}

int main()
{
    int n = 256, i, j;
    float** a, ** b, ** c;
    struct timeval start, end;
    double time_use = 0;
    printf("测试矩阵维数n = %d\n", n);

    a = (float**)malloc(n * sizeof(float*));
    b = (float**)malloc(n * sizeof(float*));
    c = (float**)malloc(n * sizeof(float*));
    for (i = 0; i < n; i++)
    {
        a[i] = (float*)malloc(n * sizeof(float));
        b[i] = (float*)malloc(n * sizeof(float));
        c[i] = (float*)malloc(n * sizeof(float));
    }

    double** a1, ** b1, ** c1;
    a1 = (double**)malloc(n * sizeof(double*));
    b1 = (double**)malloc(n * sizeof(double*));
    c1 = (double**)malloc(n * sizeof(double*));
    for (i = 0; i < n; i++)
    {
        a1[i] = (double*)malloc(n * sizeof(double));
        b1[i] = (double*)malloc(n * sizeof(double));
        c1[i] = (double*)malloc(n * sizeof(double));
    }

    for (i = 0; i < n; i++)
    {
        for (j = 0; j < n; j++)
        {
            a[i][j] = rand() % 10;
            b[i][j] = rand() % 10;
            c[i][j] = 0;
            a1[i][j] = rand() % 10;
            b1[i][j] = rand() % 10;
            c1[i][j] = 0;
        }
    }

    //SSE优化的float矩阵乘
    gettimeofday(&start, NULL);
    sse_mul_float(n, a, b, c);
    gettimeofday(&end, NULL);
    time_use = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
    printf("SSE float 类型矩阵乘枆时：%lf us\n", time_use);

    double time1 = time_use;

    //SSE优化的double矩阵乘
    gettimeofday(&start, NULL);
    sse_mul_double(n, a1, b1, c1);
    gettimeofday(&end, NULL);
    time_use = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
    printf("SSE double 类型矩阵乘枆时：%lf us\n", time_use);

    printf("speed-up = %lf 倍", time_use / time1);

    for (i = 0; i < n; i++)
    {
        free(a[i]);
        free(b[i]);
        free(c[i]);
        free(a1[i]);
        free(b1[i]);
        free(c1[i]);
    }

    free(a);
    free(b);
    free(c);
    free(a1);
    free(b1);
    free(c1);

    return 0;
}