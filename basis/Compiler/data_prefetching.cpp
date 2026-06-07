// 数据预取 (Data Prefetching)
//
// 什么是数据预取？
//   提前将数据从内存加载到缓存中，减少 CPU 等待数据的时间。
//   当 CPU 实际访问该数据时，可以直接从缓存读取，避免缓存未命中。
//
// 数据预取的作用:
//   1. 减少缓存未命中 — 数据提前进入缓存，访问时无需等待内存
//   2. 隐藏内存延迟 — 预取与计算重叠，CPU 不必空等数据
//   3. 提升访存密集型程序性能 — 对大数据集遍历效果显著
//
// __builtin_prefetch(addr, rw, locality):
//   addr     — 预取的内存地址
//   rw       — 0: 只读（读取）, 1: 读写（写入）
//   locality — 时间局部性提示（0~3）
//               0: 一次性访问，用完即丢
//               3: 频繁访问，尽量保留在缓存中
//
// 生成 LLVM IR
//   clang -emit-llvm -S -O1 data_prefetching.cpp -o data_prefetching.ll
//
// 选项:
//   -emit-llvm     生成 LLVM IR 而非本地机器码
//   -S             输出文本格式
//   -O1            启用优化（预取需要优化支持）

#include <stdio.h>

int main() {
    int arr[10];

    // 提前预取数据到缓存
    for (int i = 0; i < 10; i++) {
        __builtin_prefetch(arr + i, 1, 3);  // 读写预取，高时间局部性
    }

    // 实际访问数据时，已在缓存中
    for (int i = 0; i < 10000; i++) {
        arr[i % 10] = i;
    }

    for (int i = 0; i < 10; i++) {
        printf("%d\n", arr[i]);
    }

    return 0;
}
