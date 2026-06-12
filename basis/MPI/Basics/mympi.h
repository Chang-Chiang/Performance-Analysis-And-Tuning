/**
 * MPI 工具函数头文件
 *
 * 功能：
 * 提供 MPI 矩阵运算中常用的工具函数，包括：
 * - 矩阵初始化
 * - 矩阵乘法
 * - 矩阵分发和收集
 * - 矩阵打印和验证
 */

#ifndef MYMPI_H
#define MYMPI_H

#include <stdlib.h>
#include <time.h>
#include <mpi.h>

/* 数据类型定义，默认为 float */
#define data_t float

/**
 * 初始化矩阵
 * @param matrix 矩阵指针
 * @param length 矩阵元素总数
 * @param mod 取模值（2 表示随机 0/1，1 表示全 0）
 */
void Init_Matrix(data_t *matrix, int length, int mod)
{
    srand((int)time(0));
    int i;
    for (i = 0; i < length; i++)
    {
        matrix[i] = rand() % mod;
    }
}

/**
 * 矩阵乘法
 * @param A 输入矩阵 A (m1 x n1)
 * @param B 输入矩阵 B (n1 x n2)
 * @param C 输出矩阵 C (m1 x n2)
 * @param m1 矩阵 A 的行数
 * @param n1 矩阵 A 的列数 / 矩阵 B 的行数
 * @param n2 矩阵 B 的列数
 */
void Mul_Matrix(data_t *A, data_t *B, data_t *C, int m1, int n1, int n2)
{
    int i, j, k;
    data_t temp;
    for (i = 0; i < m1; i++)
    {
        for (j = 0; j < n2; j++)
        {
            temp = 0;
            for (k = 0; k < n1; k++)
            {
                temp += A[i * n1 + k] * B[k * n2 + j];
            }
            C[i * n2 + j] += temp;
        }
    }
}

/**
 * Cannon 算法分发矩阵
 * @param N 矩阵维度
 * @param id 当前进程 rank
 * @param num 子矩阵维度
 * @param part 进程网格维度
 * @param source_matrix 源矩阵
 * @param target_matrix 目标子矩阵
 * @param MPI_COMM_CART 笛卡尔通信域
 */
void Matrix_cannon_scatter(int N, int id, int num, int part,
                           data_t *source_matrix, data_t *target_matrix,
                           MPI_Comm MPI_COMM_CART)
{
    data_t *matrixpool;
    int i, j, w, h, count = 0;

    if (id == 0)
    {
        matrixpool = malloc(sizeof(data_t) * N * N);
        /* 将矩阵重新排序放入 matrixpool */
        for (w = 0; w < part; w++)
            for (h = 0; h < part; h++)
                for (i = w * num; i < (w + 1) * num; i++)
                    for (j = h * num; j < (h + 1) * num; j++)
                        matrixpool[count++] = source_matrix[i * N + j];
    }

    MPI_Scatter(matrixpool, num * num, MPI_FLOAT,
                target_matrix, num * num, MPI_FLOAT,
                0, MPI_COMM_CART);

    if (id == 0)
    {
        free(matrixpool);
    }
}

/**
 * Cannon 算法收集结果矩阵
 * @param N 矩阵维度
 * @param id 当前进程 rank
 * @param num 子矩阵维度
 * @param part 进程网格维度
 * @param source_c 源子矩阵
 * @param target_c 目标矩阵
 * @param MPI_COMM_CART 笛卡尔通信域
 */
void Matrix_cannon_gather(int N, int id, int num, int part,
                          data_t *source_c, data_t *target_c,
                          MPI_Comm MPI_COMM_CART)
{
    data_t *matrixpool;
    int i, j, h, w, posi;
    int temp0, temp1, temp2;

    if (id == 0)
    {
        matrixpool = malloc(sizeof(data_t) * N * N);
    }

    MPI_Gather(source_c, num * num, MPI_FLOAT,
               matrixpool, num * num, MPI_FLOAT,
               0, MPI_COMM_CART);

    if (id == 0)
    {
        posi = 0;
        for (i = 0; i < part; i++)
            for (j = 0, temp0 = i * num; j < part; j++)
                for (h = 0, temp1 = j * num; h < num; h++)
                    for (w = 0, temp2 = (temp0 + h) * N; w < num; w++)
                        target_c[temp2 + temp1 + w] = matrixpool[posi++];
        free(matrixpool);
    }
}

/**
 * 按列分解分发矩阵 A
 * @param my_id 当前进程 rank
 * @param source_matrix 源矩阵
 * @param target_matrix 目标子矩阵
 * @param dims 矩阵维度
 * @param col 每个进程处理的列数
 * @param count_p 进程数
 * @param comm 通信域
 */
void Matrix_col_scatter(int my_id, data_t *source_matrix, data_t *target_matrix,
                        int dims, int col, int count_p, MPI_Comm comm)
{
    data_t *matrix;
    int i, j, k, posi;
    int temp0, temp1;

    if (my_id == 0)
    {
        matrix = malloc(sizeof(data_t) * dims * dims);
        posi = 0;
        for (i = 0; i < count_p; i++)
            for (j = 0, temp0 = i * col; j < dims; j++)
                for (k = 0, temp1 = j * dims; k < col; k++)
                    matrix[posi++] = source_matrix[temp0 + temp1 + k];
    }

    MPI_Scatter(matrix, dims * col, MPI_FLOAT,
                target_matrix, dims * col, MPI_FLOAT,
                0, comm);

    if (my_id == 0)
    {
        free(matrix);
    }
}

/**
 * 打印矩阵
 * @param matrix 矩阵数据
 * @param m 行数
 * @param n 列数
 */
void Print_matrix(data_t *matrix, int m, int n)
{
    int i, j;
    for (i = 0; i < m; i++)
    {
        for (j = 0; j < n; j++)
        {
            printf("%f ", matrix[i * n + j]);
        }
        printf("\n");
    }
    printf("----------------------------------\n");
}

/**
 * 验证矩阵计算结果
 * @param A 输入矩阵 A
 * @param B 输入矩阵 B
 * @param C 计算结果矩阵 C
 * @param m1 矩阵 A 的行数
 * @param n1 矩阵 A 的列数
 * @param n2 矩阵 B 的列数
 */
void Verify_matrix_c(data_t *A, data_t *B, data_t *C, int m1, int n1, int n2)
{
    data_t *matrixPool;
    int i;

    matrixPool = malloc(sizeof(data_t) * m1 * n2);
    Init_Matrix(matrixPool, m1 * n2, 1);
    Mul_Matrix(A, B, matrixPool, m1, n1, n2);

    for (i = 0; i < m1 * n2; i++)
    {
        if (C[i] != matrixPool[i])
        {
            printf("计算结果错误!\n");
            if (m1 * n2 <= 36)
            {
                printf("A:\n");
                Print_matrix(A, m1, n1);
                printf("B:\n");
                Print_matrix(B, n1, n2);
                printf("C:\n");
                Print_matrix(C, m1, n2);
                printf("real-C:\n");
                Print_matrix(matrixPool, m1, n2);
            }
            free(matrixPool);
            return;
        }
    }
    printf("计算结果正确!\n");
    free(matrixPool);
}

#endif /* MYMPI_H */
