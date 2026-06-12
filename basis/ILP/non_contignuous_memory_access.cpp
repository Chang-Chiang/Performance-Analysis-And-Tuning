// 不连续访存 (Non-contiguous Memory Access)
//
// 什么是不连续访存？
//   数据在内存中的访问不是连续的，而是跨步（stride）或随机访问。
//   不连续访存会降低缓存命中率，影响性能。
//
// 不连续访存的影响：
//   1. 缓存效率低 — 跨步访问导致缓存行利用率低
//   2. 无法使用连续加载指令 — 必须使用 gather 指令
//   3. 性能下降 — 可能需要多次内存访问
//
// 解决方案：
//   1. 数据重排 — 将不连续数据重新排列为连续
//   2. 使用 gather 指令 — SIMD gather 指令收集不连续数据
//   3. 使用 shuffle 指令 — 重新排列向量中的元素
//
// 编译命令：
//   g++ -O2 -msse -Wall -o non_contignuous_memory_access non_contignuous_memory_access.cpp

#include <immintrin.h>
#include <stdio.h>

#define N 128

// 示例 1：跨步访问
void example_strided() {
    float a[N] = {0};
    float sum = 0;

    // 跨步访问：每隔 4 个元素访问一次
    for (int i = 0; i < N; i += 4) {
        a[i] = i + 2;
    }

    for (int i = 0; i < N; i++) {
        sum += a[i];
    }

    printf("Example 1 (strided): sum = %.2f\n", sum);
}

// 示例 2：SSE gather 指令收集不连续数据
void example_gather() {
    float a[N] = {0};
    float b[N] = {0};

    for (int i = 0; i < N; i += 4) {
        a[i] = i + 2;
    }

    // 使用 gather 指令收集不连续数据
    __m128 indices = _mm_set_ps(0, 0, 0, 0);
    for (int i = 0; i < N / 4; i++) {
        __m128 v = _mm_i32gather_ps(a + 4 * i, indices, sizeof(float));
        _mm_storeu_ps(b + i, v);
    }

    printf("Example 2 (gather): b[0]=%.2f, b[1]=%.2f\n", b[0], b[1]);
}

// 示例 3：标量复数乘法
void example_complex_scalar() {
    const int M = 100;
    float x[M], y[M];
    int idx1 = 0, idx2 = 4, idx3 = 0;

    for (int i = 0; i < M; i++) y[i] = i;

    // 复数乘法：(x11 + x12*i) * (x21 + x22*i)
    for (int i = 0; i < M - 4; i += 2) {
        float x11 = y[idx1 + i],     x12 = y[idx1 + i + 1];
        float x21 = y[idx2 + i],     x22 = y[idx2 + i + 1];
        x[idx3 + i]     = x11 * x21 - x12 * x22;  // 实部
        x[idx3 + i + 1] = x12 * x21 + x11 * x22;  // 虚部
    }

    printf("Example 3 (complex scalar): x[0]=%.2f, x[1]=%.2f\n", x[0], x[1]);
}

// 示例 4：SSE 向量化复数乘法
void example_complex_vector() {
    const int M = 100;
    float x[M], y[M];
    int idx1 = 0, idx2 = 4, idx3 = 0;

    for (int i = 0; i < M; i++) y[i] = i;

    // SSE 向量化复数乘法
    for (int i = 0; i < M - idx2; i += 8) {
        __m128 vx1 = _mm_loadu_ps(y + i + idx1);
        __m128 vx2 = _mm_loadu_ps(y + i + idx1 + 4);
        __m128 vxj1 = _mm_shuffle_ps(vx1, vx2, _MM_SHUFFLE(2, 0, 2, 0));  // 实部
        __m128 vxo1 = _mm_shuffle_ps(vx1, vx2, _MM_SHUFFLE(3, 1, 3, 1));  // 虚部

        __m128 vx3 = _mm_loadu_ps(y + i + idx2);
        __m128 vx4 = _mm_loadu_ps(y + i + idx2 + 4);
        __m128 vxj2 = _mm_shuffle_ps(vx3, vx4, _MM_SHUFFLE(2, 0, 2, 0));
        __m128 vxo2 = _mm_shuffle_ps(vx3, vx4, _MM_SHUFFLE(3, 1, 3, 1));

        // 复数乘法
        __m128 vy1 = _mm_mul_ps(vxj1, vxj2);  // x11 * x21
        __m128 vy2 = _mm_mul_ps(vxo1, vxo2);  // x12 * x22
        __m128 vy3 = _mm_mul_ps(vxj1, vxo2);  // x11 * x22
        __m128 vy4 = _mm_mul_ps(vxj2, vxo1);  // x21 * x12

        __m128 tp1 = _mm_sub_ps(vy1, vy2);    // 实部
        __m128 tp2 = _mm_add_ps(vy3, vy4);    // 虚部

        // 重新排列结果
        __m128 tp3 = _mm_shuffle_ps(tp1, tp2, _MM_SHUFFLE(1, 0, 1, 0));
        __m128 tp4 = _mm_shuffle_ps(tp1, tp2, _MM_SHUFFLE(3, 2, 3, 2));
        tp3 = _mm_shuffle_ps(tp3, tp3, _MM_SHUFFLE(3, 1, 2, 0));
        tp4 = _mm_shuffle_ps(tp4, tp4, _MM_SHUFFLE(3, 1, 2, 0));

        _mm_storeu_ps(x + i + idx3, tp3);
        _mm_storeu_ps(x + i + idx3 + 4, tp4);
    }

    printf("Example 4 (complex vector): x[0]=%.2f, x[1]=%.2f\n", x[0], x[1]);
}

// 示例 5：shuffle 指令重排数据
void example_shuffle() {
    const int M = 100;
    float x[M], y[M];

    for (int i = 0; i < M; i++) y[i] = i;

    // 使用 shuffle 重排数据
    for (int i = 0; i < M; i += 4) {
        __m128 vy = _mm_loadu_ps(y + i);
        __m128 vx = _mm_shuffle_ps(vy, vy, 0);  // 复制第一个元素到所有位置
        _mm_storeu_ps(&x[i], vx);
    }

    printf("Example 5 (shuffle): x[0]=%.2f, x[1]=%.2f, x[2]=%.2f, x[3]=%.2f\n",
           x[0], x[1], x[2], x[3]);
}

int main() {
    example_strided();
    example_gather();
    example_complex_scalar();
    example_complex_vector();
    example_shuffle();
    return 0;
}
