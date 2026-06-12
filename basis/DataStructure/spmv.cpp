// 稀疏矩阵向量乘法 (Sparse Matrix-Vector Multiplication, SpMV)
//
// 什么是稀疏矩阵？
//   稀疏矩阵是指大部分元素为零的矩阵。
//   直接使用二维数组存储会浪费大量内存，因此使用特殊的数据结构来存储非零元素。
//
// 本例实现四种稀疏矩阵存储格式的 SpMV：
//   1. COO（Coordinate Format）— 坐标存储
//   2. CSR（Compressed Sparse Row）— 行压缩存储
//   3. DIAG（Diagonal Format）— 对角存储
//   4. ELLPACK（Ellpack-Itpack Format）— ELLPACK 存储
//
// 存储格式比较：
//   格式      存储方式            优点                 缺点              缓存友好性
//   COO       (行,列,值) 三元组   简单直观              内存开销大        低
//   CSR       行偏移+列索引+值    内存效率高，行访问快   行操作不友好      高
//   DIAG      对角线偏移+值       规则矩阵高效          仅适用于对角矩阵  高
//   ELLPACK   每行最大非零数+索引 规则内存访问          浪费空间（补零）  中
//
// 性能差异原因：
//   1. CSR 内存连续，缓存命中率高，是最常用的稀疏矩阵格式
//   2. COO 需要多次随机访问 y[] 数组，缓存不友好
//   3. DIAG 和 ELLPACK 内存访问规则，适合向量化
//
// 编译命令：
//   clang++ -O1 spmv.cpp -o spmv

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <chrono>
#include <functional>

using namespace std;

typedef int DTYPE;
#define X -1 // 占位符，表示无数据

// 稠密矩阵向量乘法：y = A * x
void matrixvector(DTYPE* A, DTYPE* y, DTYPE* x, int n) {
    for (int i = 0; i < n; i++) {
        DTYPE y0 = 0;
        for (int j = 0; j < n; j++) {
            y0 += A[i * n + j] * x[j];
        }
        y[i] = y0;
    }
}

// COO 格式 SpMV：使用 (行,列,值) 三元组
void spmv_coo(int* row, int* col, DTYPE* values, DTYPE* y, DTYPE* x, int nnz) {
    for (int i = 0; i < nnz; i++) {
        y[row[i]] += values[i] * x[col[i]];
    }
}

// CSR 格式 SpMV：使用行偏移、列索引、值
void spmv_csr(int* ptr, int* col, DTYPE* data, DTYPE* y, DTYPE* x, int num_rows) {
    for (int i = 0; i < num_rows; i++) {
        DTYPE y0 = 0;
        for (int k = ptr[i]; k < ptr[i + 1]; k++) {
            y0 += data[k] * x[col[k]];
        }
        y[i] = y0;
    }
}

// 生成随机稀疏矩阵（CSR 格式）
void generate_sparse_matrix_csr(
    int n, double density, int** ptr, int** col, DTYPE** data, int* nnz) {
    int max_nnz = (int)(n * n * density);
    *ptr        = (int*)malloc((n + 1) * sizeof(int));
    *col        = (int*)malloc(max_nnz * sizeof(int));
    *data       = (DTYPE*)malloc(max_nnz * sizeof(DTYPE));

    int count = 0;
    (*ptr)[0] = 0;

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            if ((double)rand() / RAND_MAX < density) {
                (*col)[count]  = j;
                (*data)[count] = rand() % 100 + 1;
                count++;
            }
        }
        (*ptr)[i + 1] = count;
    }
    *nnz = count;
}

// 从 CSR 格式转换为 COO 格式
void csr_to_coo(
    int* ptr, int* col, DTYPE* data, int n, int nnz, int** row_coo, int** col_coo,
    DTYPE** data_coo) {
    *row_coo  = (int*)malloc(nnz * sizeof(int));
    *col_coo  = (int*)malloc(nnz * sizeof(int));
    *data_coo = (DTYPE*)malloc(nnz * sizeof(DTYPE));

    int count = 0;
    for (int i = 0; i < n; i++) {
        for (int k = ptr[i]; k < ptr[i + 1]; k++) {
            (*row_coo)[count]  = i;
            (*col_coo)[count]  = col[k];
            (*data_coo)[count] = data[k];
            count++;
        }
    }
}

// ELLPACK 格式 SpMV
void spmv_ellpack(DTYPE* data, int* indices, DTYPE* y, DTYPE* x, int n, int max_cols) {
    for (int j = 0; j < max_cols; j++) {
        for (int i = 0; i < n; i++) {
            int idx = j * n + i;
            if (data[idx] != 0) {
                y[i] += data[idx] * x[indices[idx]];
            }
        }
    }
}

// 从 CSR 格式转换为 ELLPACK 格式
void csr_to_ellpack(
    int* ptr, int* col, DTYPE* data, int n, DTYPE** data_ell, int** indices_ell, int* max_cols) {
    // 计算每行最大非零元素数
    *max_cols = 0;
    for (int i = 0; i < n; i++) {
        int row_nnz = ptr[i + 1] - ptr[i];
        if (row_nnz > *max_cols) {
            *max_cols = row_nnz;
        }
    }

    // 分配内存并填充
    *data_ell    = (DTYPE*)malloc(n * (*max_cols) * sizeof(DTYPE));
    *indices_ell = (int*)malloc(n * (*max_cols) * sizeof(int));
    memset(*data_ell, 0, n * (*max_cols) * sizeof(DTYPE));
    memset(*indices_ell, 0, n * (*max_cols) * sizeof(int));

    for (int i = 0; i < n; i++) {
        int k = 0;
        for (int j = ptr[i]; j < ptr[i + 1]; j++) {
            (*data_ell)[k * n + i]    = data[j];
            (*indices_ell)[k * n + i] = col[j];
            k++;
        }
    }
}

// 对角格式 SpMV
void spmv_diag(DTYPE* data, int* offsets, DTYPE* y, DTYPE* x, int n, int num_diags) {
    for (int d = 0; d < num_diags; d++) {
        int k         = offsets[d];
        int row_start = (k >= 0) ? 0 : -k;
        int col_start = (k >= 0) ? k : 0;
        int len       = n - abs(k);

        for (int i = 0; i < len; i++) {
            y[row_start + i] += data[d * n + i] * x[col_start + i];
        }
    }
}

// 从 CSR 格式转换为对角格式
void csr_to_diag(
    int* ptr, int* col, DTYPE* data_csr, int n, DTYPE** data_diag, int** offsets, int* num_diags) {
    // 统计所有对角线
    int* diag_count = (int*)malloc((2 * n - 1) * sizeof(int));
    memset(diag_count, 0, (2 * n - 1) * sizeof(int));

    for (int i = 0; i < n; i++) {
        for (int k = ptr[i]; k < ptr[i + 1]; k++) {
            int j    = col[k];
            int diag = j - i + (n - 1); // 映射到 [0, 2n-2]
            diag_count[diag]++;
        }
    }

    // 统计非零对角线数量
    *num_diags = 0;
    for (int d = 0; d < 2 * n - 1; d++) {
        if (diag_count[d] > 0) {
            (*num_diags)++;
        }
    }

    // 分配内存
    *data_diag = (DTYPE*)malloc((*num_diags) * n * sizeof(DTYPE));
    *offsets   = (int*)malloc((*num_diags) * sizeof(int));
    memset(*data_diag, 0, (*num_diags) * n * sizeof(DTYPE));

    // 填充数据
    int diag_idx = 0;
    for (int d = 0; d < 2 * n - 1; d++) {
        if (diag_count[d] > 0) {
            int offset = d - (n - 1);
            (*offsets)[diag_idx] = offset;

            // 计算该对角线的起始行和长度
            int row_start = (offset >= 0) ? 0 : -offset;
            int len = n - abs(offset);

            // 填充该对角线的数据（从索引 0 开始存储）
            for (int i = 0; i < len; i++) {
                int row = row_start + i;
                int col_idx = row + offset;
                // 在 CSR 中查找 (row, col_idx)
                for (int k = ptr[row]; k < ptr[row + 1]; k++) {
                    if (col[k] == col_idx) {
                        (*data_diag)[diag_idx * n + i] = data_csr[k];
                        break;
                    }
                }
            }
            diag_idx++;
        }
    }

    free(diag_count);
}

// 测试算法性能
void benchmark(const char* name, std::function<void()> func, int iterations) {
    // 预热
    func();

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; i++) {
        func();
    }
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    printf("│ %-12s │ %10d │ %16ld │\n", name, iterations, duration.count());
}

int main() {
    int    n          = 1000; // 矩阵大小
    double density    = 0.01; // 稀疏度（非零元素比例）
    int    iterations = 1000; // 迭代次数

    // 生成稀疏矩阵
    int *ptr, *col, *data;
    int  nnz;
    generate_sparse_matrix_csr(n, density, &ptr, &col, &data, &nnz);

    // 转换为 COO 格式
    int *  row_coo, *col_coo;
    DTYPE* data_coo;
    csr_to_coo(ptr, col, data, n, nnz, &row_coo, &col_coo, &data_coo);

    // 转换为 ELLPACK 格式
    DTYPE* data_ell;
    int*   indices_ell;
    int    max_cols;
    csr_to_ellpack(ptr, col, data, n, &data_ell, &indices_ell, &max_cols);

    // 转换为对角格式
    DTYPE* data_diag;
    int*   offsets_diag;
    int    num_diags;
    csr_to_diag(ptr, col, data, n, &data_diag, &offsets_diag, &num_diags);

    // 初始化向量
    DTYPE* x       = (DTYPE*)malloc(n * sizeof(DTYPE));
    DTYPE* y_dense = (DTYPE*)malloc(n * sizeof(DTYPE));
    DTYPE* y_coo   = (DTYPE*)malloc(n * sizeof(DTYPE));
    DTYPE* y_csr   = (DTYPE*)malloc(n * sizeof(DTYPE));
    DTYPE* y_ell   = (DTYPE*)malloc(n * sizeof(DTYPE));
    DTYPE* y_diag  = (DTYPE*)malloc(n * sizeof(DTYPE));

    for (int i = 0; i < n; i++) {
        x[i] = rand() % 100 + 1;
    }

    // 生成稠密矩阵（用于对比）
    DTYPE* A = (DTYPE*)malloc(n * n * sizeof(DTYPE));
    memset(A, 0, n * n * sizeof(DTYPE));
    for (int i = 0; i < nnz; i++) {
        A[row_coo[i] * n + col_coo[i]] = data_coo[i];
    }

    printf("SpMV Performance Benchmark:\n");
    printf("Matrix size: %d x %d, Non-zeros: %d, Density: %.2f%%\n", n, n, nnz, density * 100);
    printf("┌──────────────┬────────────┬──────────────────┐\n");
    printf("│ Format       │ Iterations │      Time (us)   │\n");
    printf("├──────────────┼────────────┼──────────────────┤\n");

    // 测试稠密矩阵向量乘法
    benchmark("Dense", [&]() { matrixvector(A, y_dense, x, n); }, iterations);

    // 测试 COO 格式
    benchmark(
        "COO",
        [&]() {
            memset(y_coo, 0, n * sizeof(DTYPE));
            spmv_coo(row_coo, col_coo, data_coo, y_coo, x, nnz);
        },
        iterations);

    // 测试 CSR 格式
    benchmark(
        "CSR",
        [&]() {
            memset(y_csr, 0, n * sizeof(DTYPE));
            spmv_csr(ptr, col, data, y_csr, x, n);
        },
        iterations);

    // 测试 ELLPACK 格式
    benchmark(
        "ELLPACK",
        [&]() {
            memset(y_ell, 0, n * sizeof(DTYPE));
            spmv_ellpack(data_ell, indices_ell, y_ell, x, n, max_cols);
        },
        iterations);

    // 测试对角格式
    benchmark(
        "DIAG",
        [&]() {
            memset(y_diag, 0, n * sizeof(DTYPE));
            spmv_diag(data_diag, offsets_diag, y_diag, x, n, num_diags);
        },
        iterations);

    printf("└──────────────┴────────────┴──────────────────┘\n");

    // 验证结果一致性
    printf("\nVerification:\n");
    int pass = 1;
    for (int i = 0; i < n; i++) {
        if (y_dense[i] != y_coo[i] || y_dense[i] != y_csr[i] || y_dense[i] != y_ell[i] ||
            y_dense[i] != y_diag[i]) {
            printf(
                "  FAILED at index %d: dense=%d, coo=%d, csr=%d, ell=%d, diag=%d\n", i, y_dense[i],
                y_coo[i], y_csr[i], y_ell[i], y_diag[i]);
            pass = 0;
            break;
        }
    }
    if (pass) {
        printf("  All formats produce the same result.\n");
    }

    // 释放内存
    free(ptr);
    free(col);
    free(data);
    free(row_coo);
    free(col_coo);
    free(data_coo);
    free(data_ell);
    free(indices_ell);
    free(data_diag);
    free(offsets_diag);
    free(x);
    free(y_dense);
    free(y_coo);
    free(y_csr);
    free(y_ell);
    free(y_diag);
    free(A);

    return 0;
}
