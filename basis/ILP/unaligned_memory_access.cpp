// 不对齐访存 (Unaligned Memory Access)
//
// 什么是不对齐访存？
//   数据在内存中的地址不是向量宽度的整数倍时，称为不对齐访存。
//   不对齐访存可能需要多次内存访问，降低性能。
//
// 不对齐访存的影响：
//   1. 性能下降 — 可能需要两次内存访问才能加载一个向量
//   2. 缓存效率低 — 跨缓存行访问增加缓存未命中
//   3. 无法使用对齐加载指令 — 必须使用未对齐加载指令
//
// 解决方案：
//   1. 数据填充 — 增加数组大小使其对齐
//   2. 循环分段 — 将循环分为对齐部分和未对齐部分
//   3. 使用未对齐加载指令 — _mm_loadu_ps 而非 _mm_load_ps
//
// 编译命令：
//   g++ -O2 -msse -Wall -o unaligned_memory_access unaligned_memory_access.cpp

#include <immintrin.h>
#include <stdio.h>

// 示例 1：不对齐访存
void example_1() {
    const int N = 100;
    float     A[N], B[N];
    float     C = 0.5f;

    for (int i = 0; i < N; i++) {
        A[i] = i * 2;
    }

    B[0] = 0;
    for (int i = 0; i < N; i++) {
        B[i + 1] = A[i + 1] + C; // 不对齐访问
    }

    printf("Example 1: B[0]=%f, B[1]=%f\n", B[0], B[1]);
}

// 示例 2：循环分段处理不对齐访存
void example_2() {
    const int N = 100;
    float     A[N], B[N];
    float     C = 0.5f;

    for (int i = 0; i < N; i++) {
        A[i] = i * 2;
    }

    B[0] = 0;
    for (int i = 0; i < 3; i++) {
        B[i + 1] = A[i + 1] + C; // 头部
    }
    for (int i = 3; i < 99; i++) {
        B[i + 1] = A[i + 1] + C; // 主体
    }
    for (int i = 99; i < 100; i++) {
        B[i + 1] = A[i + 1] + C; // 尾部
    }

    printf("Example 2: B[0]=%f, B[1]=%f\n", B[0], B[1]);
}

// 示例 3：大矩阵不对齐访存
void example_3() {
    const int N = 1335;
    float     A[N][N];

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            A[i][j] = i + j;
        }
    }

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            A[i][j] = A[i][j] * 2;
        }
    }

    printf("Example 3: A[0][0]=%f\n", A[0][0]);
}

// 示例 4：大矩阵填充对齐
void example_4() {
    const int N = 1335;
    float     A[N][N + 1]; // 填充一列使其对齐

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N + 1; j++) {
            A[i][j] = i + j;
        }
    }

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N + 1; j++) {
            A[i][j] = A[i][j] * 2;
        }
    }

    printf("Example 4: A[0][0]=%f\n", A[0][0]);
}

// 示例 5：标量拷贝（带偏移）
void example_5() {
    const int N = 100;
    float     A[N + 2], C[N];

    for (int i = 0; i < N + 2; i++) {
        A[i] = i;
    }

    for (int i = 0; i < N; i++) {
        C[i] = A[i + 2]; // 偏移 2 个元素
    }

    printf("Example 5: C[0]=%f, C[1]=%f\n", C[0], C[1]);
}

// 示例 6：SSE 向量化拷贝（带偏移）
void example_6() {
    const int N = 100;
    float     A[N + 2], C[N];

    for (int i = 0; i < N + 2; i++) {
        A[i] = i;
    }

    // 使用未对齐加载指令
    for (int i = 0; i < N; i += 4) {
        __m128 va = _mm_loadu_ps(&A[i + 2]);
        _mm_storeu_ps(&C[i], va);
    }

    printf("Example 6: C[0]=%f, C[1]=%f\n", C[0], C[1]);
}

int main() {
    example_1();
    example_2();
    example_3();
    example_4();
    example_5();
    example_6();
    return 0;
}
