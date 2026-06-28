# TMA (Top-down Microarchitecture Analysis)

基于 [perf-ninja](https://github.com/dendibakh/perf-ninja) 的 TMA 实战练习。

## 环境搭建

### 1. Google Benchmark 工具下载

```bash
cd TMA
./tools/make_benchmark_library.sh
```

### 2. CPU 性能模式

```bash
# 锁定 CPU 最高频率，避免动态调频干扰 benchmark
sudo cpupower frequency-set --governor performance
```

### 3. Perf 权限设置

```bash
# 允许 perf 访问 CPU PMU 事件（Top-Down 分析需要）
sudo sysctl -w kernel.perf_event_paranoid=0

# 永久生效
echo "kernel.perf_event_paranoid = 0" | sudo tee -a /etc/sysctl.conf
sudo sysctl -p
```

### 4. 编译与运行

```bash
# 进入任意实验目录（以 data_packing 为例）
cd memory_bound/data_packing

# 配置构建
cmake -E make_directory build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_FLAGS="-g" -DCMAKE_CXX_FLAGS="-g" ..

# 编译
cmake --build . --config Release --parallel 8

# 验证正确性
./validate

# 运行 benchmark
cmake --build . --target benchmarkLab

# TMA 分析
perf stat --topdown -a taskset -c 0 ./lab
python3 ~/pmu-tools/toplev.py --core S0-C0 -l2 --no-desc taskset -c 0 ./lab
```

## 实验列表

### Memory Bound 内存瓶颈

| 实验 | 目录 | 优化技术 | 加速比 |
|------|------|---------|--------|
| Data Packing | [`memory_bound/data_packing/`](memory_bound/data_packing/) | 结构体压缩 | 7.5x |
| Loop Interchange 1 | [`memory_bound/loop_interchange_1/`](memory_bound/loop_interchange_1/) | 矩阵乘法循环交换 | 6.9x |
| Loop Interchange 2 | [`memory_bound/loop_interchange_2/`](memory_bound/loop_interchange_2/) | 高斯模糊循环交换 | 9.7x |
| SW Memory Prefetch | [`memory_bound/swmem_prefetch_1/`](memory_bound/swmem_prefetch_1/) | 软件预取 | 3.1x |
| Loop Tiling | [`memory_bound/loop_tiling_1/`](memory_bound/loop_tiling_1/) | 循环分块 | 1.75x |
| False Sharing | [`memory_bound/false_sharing_1/`](memory_bound/false_sharing_1/) | 消除伪共享 | 15.8x |

### Core Bound 计算瓶颈

| 实验 | 目录 | 优化技术 | 加速比 |
|------|------|---------|--------|
| Vectorization 1 | [`core_bound/vectorization_1/`](core_bound/vectorization_1/) | AoS → SoA 自动向量化 | 2.1x |
| Vectorization 2 | [`core_bound/vectorization_2/`](core_bound/vectorization_2/) | 消除依赖链 | 20.9x |
| Function Inlining | [`core_bound/function_inlining_1/`](core_bound/function_inlining_1/) | 函数内联 | 1.63x |
| Compiler Intrinsics 1 | [`core_bound/compiler_intrinsics_1/`](core_bound/compiler_intrinsics_1/) | SSE4.1 前缀和 | 1.14x |
| Compiler Intrinsics 2 | [`core_bound/compiler_intrinsics_2/`](core_bound/compiler_intrinsics_2/) | AVX2 批量查找 | 15.1x |
| Dependency Chains | [`core_bound/dep_chains_1/`](core_bound/dep_chains_1/) | 并行依赖链 | 3.6x |

### Bad Speculation 分支预测

| 实验 | 目录 | 优化技术 | 加速比 |
|------|------|---------|--------|
| Lookup Tables | [`bad_speculation/lookup_tables_1/`](bad_speculation/lookup_tables_1/) | 查找表替代分支 | 1.87x |

### Misc 编译器优化

| 实验 | 目录 | 优化技术 | 加速比 |
|------|------|---------|--------|
| LTO | [`misc/lto/`](misc/lto/) | 链接时优化 | 1.50x |
| PGO | [`misc/pgo/`](misc/pgo/) | Profile-Guided Optimization | ~1x |
| Warmup | [`misc/warmup/`](misc/warmup/) | 算法优化 O(N)→O(1) | 70x |
