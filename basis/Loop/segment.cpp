// 循环分段, 将单层循环变换为嵌套循环

#include <stdio.h>
#include <x86intrin.h>

#define N 256

int main()
{
    int i;
    float A[N], B[N], C[N];
    for (i = 1; i < N; i++)
    {
        A[i] = 1.0;
        B[i] = 2.0;
        C[i] = 3.0;
    }

    // for (i = 0; i < N; i++)
    // {
    //     A[i] = B[i] + C[i];
    // }

    __m128 ymm0, ymm1, ymm2, ymm3;
    int K = 32;
    for (i = 0; i < N; i += K)
    {
        for (j = i; j < i + K - 1; j += 4)
        {
            ymm0 = _mm_load_ps(B + j);
            ymm1 = _mm_load_ps(C + j);
            ymm2 = _mm_add_ps(ymm0, ymm1);
            _mm_storeu_ps(A + j, ymm2);
        }
    }
    for (i = 0; i < N; i++)
    {
        printf("%f  ", A[i]);
    }
}