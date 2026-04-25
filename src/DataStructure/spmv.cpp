// spmv, sparse matrix vector multiply, 稀疏矩阵 矩阵向量乘

#include <stdio.h>
#include <algorithm>
using namespace std;

const static int SIZE = 4;      // 矩阵的大小
const static int NNZ = 9;       // 非零元素的数量
const static int NUM_ROWS = 4;  // 列长;
typedef int DTYPE;
#define X -1

// y (4*1) = A (4*4) * x (4*1)
void matrixvector(int A[SIZE][SIZE], DTYPE* y, DTYPE* x)
{
    for (int i = 0; i < SIZE; i++)
    {
        DTYPE y0 = 0;
        for (int j = 0; j < SIZE; j++)
        {
            y0 += A[i][j] * x[j];
        }
        y[i] = y0;
    }
}

// row: 非零元素所在的行索引
// col: 非零元素所在的列索引
// values: 非零元素的值
// y: 结果向量
// x: 输入向量
void spmv_coo(int row[NNZ], int col[NNZ], DTYPE values[NNZ], DTYPE y[SIZE], DTYPE x[SIZE])
{
    int i;
    for (i = 0; i < SIZE; i++)
    {
        y[i] = 0;
    }
    for (i = 0; i < NNZ; i++)
    {
        y[row[i]] += values[i] * x[col[i]];
    }
}

// CSR 行压缩存储的矩阵向量乘
void spmv_csr(int ptr[NUM_ROWS + 1], int col[NNZ], DTYPE data[NNZ], DTYPE y[SIZE], DTYPE x[SIZE])
{
    for (int i = 0; i < NUM_ROWS; i++)
    {
        DTYPE y0 = 0;
        for (int k = ptr[i]; k < ptr[i + 1]; k++)
        {
            y0 += data[k] * x[col[k]];
        }
        y[i] = y0;
    }
}

// 对角存储的矩阵向量乘
void spmv_diag(DTYPE data[12], int offsets[SIZE - 1], DTYPE y[SIZE], DTYPE x[SIZE])
{
    int i, j, k, N;
    int Istart, Jstart, stride = 4;

    for (i = 0; i < SIZE - 1; i++)
    {
        k = offsets[i];
        Istart = max(0, -k);
        Jstart = max(0, k);
        N = min(SIZE - Istart, SIZE - Jstart);
        for (j = 0; j < N; j++)
        {
            if (data[Istart + i * stride + j] != X)
            {
                y[Istart + j] += data[Istart + i * stride + j] * x[Jstart + j];
            }
        }
    }
}

// ELLPACK 存储的矩阵向量乘
void spmv_ellpack(DTYPE data[12], int indices[12], DTYPE y[SIZE], DTYPE x[SIZE])
{
    int n, i, k, N;
    int max_ncols = SIZE - 1, num_rows = SIZE;
    for (n = 0; n < max_ncols; n++)
    {
        for (i = 0; i < num_rows; i++)
        {
            if (data[n * num_rows + i] != X)
            {
                y[i] += data[n * num_rows + i] * x[indices[n * num_rows + i]];
            }
        }
    }
}

int main()
{
    int fail_spmv_coo = 0;
    int fail_spmv_csr = 0;
    int fail_spmv_diag = 0;
    int fail_spmv_ellpack = 0;

    DTYPE M[SIZE][SIZE] = { {1, 5, 0, 0}, 
                            {0, 2, 6, 0}, 
                            {8, 0, 3, 7}, 
                            {0, 9, 0, 4} };
    DTYPE x[SIZE] = { 1, 2, 3, 4 };
    DTYPE y_sw[SIZE] = { 0, 0, 0, 0 };
    DTYPE y_coo[SIZE] = { 0, 0, 0, 0 };
    DTYPE y_csr[SIZE] = { 0, 0, 0, 0 };
    DTYPE y_diag[SIZE] = { 0, 0, 0, 0 };
    DTYPE y_ellpack[SIZE] = { 0, 0, 0, 0 };

    // 稀疏矩阵的 坐标存储 表示
    DTYPE values_coo[] = { 1, 5, 2, 6, 8, 3, 7, 9, 4 };
    int col_coo[] = { 0, 1, 1, 2, 0, 2, 3, 1, 3 };
    int row_coo[] = { 0, 0, 1, 1, 2, 2, 2, 3, 3 };
    spmv_coo(row_coo, col_coo, values_coo, y_coo, x);

    // 稀疏矩阵的 CSR 存储表示
    DTYPE values_csr[] = { 1, 5, 2, 6, 8, 3, 7, 9, 4 };
    int col_csr[] = { 0, 1, 1, 2, 0, 2, 3, 1, 3 };
    int ptr[] = { 0, 2, 4, 7, 9 };  // 每行第一个非 0 元素, 在 values 中的存储位置
    spmv_csr(ptr, col_csr, values_csr, y_csr, x);

    // 稀疏矩阵的 对角存储表示
    DTYPE data_diag[12] = { X, X, 8, 9, 1, 2, 3, 4, 5, 6, 7, X };
    int offsets[SIZE - 1] = { -2, 0, 1 };
    spmv_diag(data_diag, offsets, y_diag, x);

    // ellpack 存储表示
    DTYPE data_ellpack[12] = { 1, 2, 8, 9, 5, 6, 3, 4, X, X, 7, X };
    DTYPE indices_ellpack[12] = { 0, 1, 0, 1, 1, 2, 2, 3, X, X, 3, X };
    spmv_ellpack(data_ellpack, indices_ellpack, y_ellpack, x);
    matrixvector(M, y_sw, x);

    //判断两次矩阵相量乘的结果是否一样
    for (int i = 0; i < SIZE; i++)
    {
        if (y_sw[i] != y_coo[i]) { fail_spmv_coo = 1; }
        if (y_sw[i] != y_csr[i]) { fail_spmv_csr = 1; }
        if (y_sw[i] != y_diag[i]) { fail_spmv_diag = 1; }
        if (y_sw[i] != y_ellpack[i]) { fail_spmv_ellpack = 1; }
    }

    if (fail_spmv_coo == 1) { printf("COO SPMV FAILED\n"); }
    else { printf("COO SPMV PASS\n"); }

    if (fail_spmv_csr == 1) { printf("CSR SPMV FAILED\n"); }
    else { printf("CSR SPMV PASS\n"); }

    if (fail_spmv_diag == 1) { printf("DIAG SPMV FAILED\n"); }
    else { printf("DIAG SPMV PASS\n"); }

    if (fail_spmv_ellpack == 1) { printf("ELLPACK SPMV FAILED\n"); }
    else { printf("ELLPACK SPMV PASS\n"); }

    printf("矩阵M为: \n");

    for (int i = 0; i < SIZE; i++)
    {
        for (int j = 0; j < SIZE; j++)
        {
            printf("%d  ", M[i][j]);
        }
        printf("\n");
    }

    printf("向量X为: \n");
    for (int i = 0; i < SIZE; i++)
    {
        printf("%d ", x[i]);
    }
    printf("\n");

    printf("矩阵向量乘的积为: \n");
    for (int i = 0; i < SIZE; i++)
    {
        printf("%d ", y_sw[i]);
    }
    printf("\n");

    return fail_spmv_coo && fail_spmv_csr && fail_spmv_diag && fail_spmv_ellpack;
}