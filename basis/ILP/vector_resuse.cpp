// 向量重用 (Vector Reuse)
//
// 什么是向量重用？
//   将已加载到向量寄存器中的数据重复使用，避免重复加载。
//   向量重用可以减少内存访问次数，提升性能。
//
// 向量重用的作用：
//   1. 减少内存访问 — 数据在寄存器中重用，无需重新加载
//   2. 提升缓存效率 — 减少缓存未命中
//   3. 降低内存带宽压力 — 减少对内存总线的访问
//
// 示例分析：
//   优化前（重复加载）：
//     for (i = 0; i < N; i += 4) {
//         vb = _mm_loadu_ps(b + i);  // 第一次加载
//         vc = _mm_loadu_ps(c + i);
//         va = _mm_add_ps(vb, vc);
//         _mm_storeu_ps(a + i, va);
//         vb = _mm_loadu_ps(b + i);  // 重复加载
//         vc = _mm_loadu_ps(c + i);  // 重复加载
//         vd = _mm_sub_ps(vb, vc);
//         _mm_storeu_ps(d + i, vd);
//     }
//
//   优化后（向量重用）：
//     for (i = 0; i < N; i += 4) {
//         vb = _mm_loadu_ps(b + i);  // 只加载一次
//         vc = _mm_loadu_ps(c + i);  // 只加载一次
//         va = _mm_add_ps(vb, vc);
//         _mm_storeu_ps(a + i, va);
//         vd = _mm_sub_ps(vb, vc);   // 重用 vb, vc
//         _mm_storeu_ps(d + i, vd);
//     }
//
// 编译命令：
//   g++ -O2 -msse -Wall -o vector_resuse vector_resuse.cpp

#include <immintrin.h>
#include <stdio.h>

#define N 100

// 示例 1：shuffle 重排数据
void example_shuffle() {
    float a[N + 2], c[N];

    for (int i = 0; i < N + 2; i++) a[i] = i;

    // 使用 shuffle 重排数据
    __m128 va1 = _mm_loadu_ps(a);
    for (int i = 0; i < N; i += 4) {
        __m128 va2 = _mm_loadu_ps(&a[i + 4]);
        __m128 v   = _mm_shuffle_ps(va1, va2, _MM_SHUFFLE(1, 0, 3, 2));
        _mm_storeu_ps(&c[i], v);
        va1 = va2;
    }

    printf("Example 1 (shuffle): c[0]=%.2f, c[1]=%.2f\n", c[0], c[1]);
}

// 示例 2：标量向量重用
void example_scalar() {
    float a[N], b[N], c[N], d[N];

    for (int i = 0; i < N; i++) {
        b[i] = i * 2;
        c[i] = i;
    }

    // 标量计算：b 和 c 被重用
    for (int i = 0; i < N; i++) {
        a[i] = b[i] + c[i];
        d[i] = b[i] - c[i];
    }

    printf("Example 2 (scalar): a[0]=%.2f, d[0]=%.2f\n", a[0], d[0]);
}

// 示例 3：SSE 向量（重复加载）
void example_vector_reload() {
    float a[N], b[N], c[N], d[N];

    for (int i = 0; i < N; i++) {
        b[i] = i * 2;
        c[i] = i;
    }

    // 重复加载：b 和 c 被加载两次
    for (int i = 0; i < N; i += 4) {
        __m128 vb = _mm_loadu_ps(b + i);
        __m128 vc = _mm_loadu_ps(c + i);
        __m128 va = _mm_add_ps(vb, vc);
        _mm_storeu_ps(a + i, va);

        vb = _mm_loadu_ps(b + i);  // 重复加载
        vc = _mm_loadu_ps(c + i);  // 重复加载
        __m128 vd = _mm_sub_ps(vb, vc);
        _mm_storeu_ps(d + i, vd);
    }

    printf("Example 3 (reload): a[0]=%.2f, d[0]=%.2f\n", a[0], d[0]);
}

// 示例 4：SSE 向量重用
void example_vector_reuse() {
    float a[N], b[N], c[N], d[N];

    for (int i = 0; i < N; i++) {
        b[i] = i * 2;
        c[i] = i;
    }

    // 向量重用：b 和 c 只加载一次
    for (int i = 0; i < N; i += 4) {
        __m128 vb = _mm_loadu_ps(b + i);  // 只加载一次
        __m128 vc = _mm_loadu_ps(c + i);  // 只加载一次
        __m128 va = _mm_add_ps(vb, vc);
        _mm_storeu_ps(a + i, va);
        __m128 vd = _mm_sub_ps(vb, vc);   // 重用 vb, vc
        _mm_storeu_ps(d + i, vd);
    }

    printf("Example 4 (reuse):  a[0]=%.2f, d[0]=%.2f\n", a[0], d[0]);
}

int main() {
    example_shuffle();
    example_scalar();
    example_vector_reload();
    example_vector_reuse();
    return 0;
}
