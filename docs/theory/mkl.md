# Intel MKL 数学库

Intel **MKL**（Math Kernel Library，现称 **oneMKL**）是 Intel 提供、针对 Intel/AMD x86 处理器深度优化的商用级数值计算库。它把「线性代数、向量数学、傅里叶变换、线性方程组、随机数」等重型数学内核做到接近硬件峰值，是**性能敏感数值计算的首选底座**——正如 [`04-编译器概述.md`](../notes/04-编译器概述.md) 所强调：**能交给数学库的，就别自己写循环**。

本文是 MKL 的独立参考文档：从概念、接口约定、链接方式，到线程/内存/指令集控制与调优清单，并配套可跑示例 [`basis/Compiler/MKL/`](../../basis/Compiler/MKL/)。

- [Intel MKL 数学库](#intel-mkl-数学库)
  - [1. 为什么用 MKL](#1-为什么用-mkl)
  - [2. 组件全景](#2-组件全景)
  - [3. 接口与命名约定](#3-接口与命名约定)
  - [4. 三种链接方式](#4-三种链接方式)
  - [5. 线程与并行控制](#5-线程与并行控制)
  - [6. 内存对齐与分配器](#6-内存对齐与分配器)
  - [7. BLAS：稠密线性代数](#7-blas稠密线性代数)
  - [8. VML：向量数学函数](#8-vml向量数学函数)
  - [9. LAPACK：高阶数值算法](#9-lapack高阶数值算法)
  - [10. FFT / DFTI：离散傅里叶变换](#10-fft--dfti离散傅里叶变换)
  - [11. 环境变量速查](#11-环境变量速查)
  - [12. 调优清单与常见陷阱](#12-调优清单与常见陷阱)
  - [13. 示例代码与延伸阅读](#13-示例代码与延伸阅读)

---

## 1. 为什么用 MKL

| 维度 | 自己写循环 | MKL 打包内核 |
|------|-----------|-------------|
| 向量化 | 依赖编译器 `-O3` 自动向量化，遇别名/归约易失败 | 手写 SIMD（SSE/AVX2/AVX-512）+ FMA，稳定吃满 |
| 分块复用 | 需手动 tiling | 内核自动多级分块，命中 L1/L2/寄存器 |
| 并行 | 需 OpenMP/Pthread 手工分派 | 库内线程自动并行（可关） |
| 数值稳定 | 易踩溢出/主元问题 | LU 部分主元、缩放等成熟实现 |
| 可移植 | — | 运行期按 CPU 指令集**动态派发**最优内核 |

本项目实测（单线程，见 [`basis/Compiler/MKL/`](../../basis/Compiler/MKL/)）：`cblas_dgemm` 1000×1000 矩阵乘约 **69 GFLOPS**，比手写 i-k-j 三重循环（~13 GFLOPS）快 **~5x**；`vdLn` 数组对数比标量 `log()` 循环快 **4~6x** 且精度可控。

> **一句话**：MKL 的价值是「把每个算子内核调到接近 Roofline 屋顶」。你负责选对接口、给对齐内存、控好线程；它负责极限算力。

---

## 2. 组件全景

| 组件 | 提供什么 | C 头文件 | 典型符号 |
|------|----------|----------|----------|
| **BLAS** | 稠密向量/矩阵运算（Level 1/2/3） | `mkl_cblas.h` | `cblas_dgemm` `cblas_daxpy` |
| **LAPACK** | 矩阵分解、方程组、特征值、SVD | `mkl_lapacke.h` | `LAPACKE_dgesv` `LAPACKE_dgetrf` |
| **VML** | 数组级初等函数（sin/cos/ln/exp…） | `mkl_vml_functions.h` | `vdLn` `vsSin` |
| **FFT / DFTI** | 一维/多维离散傅里叶变换 | `mkl_dfti.h` | `DftiComputeForward` |
| **Sparse Solver** | 稀疏矩阵直接/迭代求解 | `mkl_sparsity.h` `mkl_types.h` | `mkl_sparse_d_mv` |
| **VSL（RNG）** | 伪随机数、蒙特卡洛 | `mkl_vsl_functions.h` | `vldBRng` |
| **BLACS / ScaLAPACK** | 分布式（MPI）稠密线性代数 | `mkl_blacs.h` | `Cblacs_gridinit` |

`-lmkl_rt`（运行时动态库）把 BLAS/LAPACK/VML/FFT 打包进**一个** so，并按 CPU 动态派发内核，是最省事的链接方式。

---

## 3. 接口与命名约定

### 3.1 数据类型前缀（BLAS / LAPACK / VML 通用）

| 前缀 | 类型 | 例 |
|------|------|-----|
| `s` | `float` | `sgemm` / `vsLn` |
| `d` | `double` | `dgemm` / `vdLn` |
| `c` | `complex<float>` | `cgemm` / `vcLn` |
| `z` | `complex<double>` | `zgemm` / `vdLn`（复数版 `vc/vz`） |

BLAS 名 = **精度字母** + **运算缩写**。`dgemm` = **d**ouble + **ge**neral matrix + **m**atrix **m**ultiply。

### 3.2 行优先 vs 列优先

- **经典 Fortran BLAS/LAPACK**：一律**列优先**（column-major），参数按位置传。
- **C 接口**：
  - `cblas_*`：可显式指定 `CblasRowMajor` / `CblasColMajor`，索引从 0 开始，更贴合 C/C++。
  - `mkl_*`（如 `mkl_dgemm`）：MKL 原生 Fortran 风格接口，仅列优先。
  - `LAPACKE_*`：LAPACK 的 C 封装，可指定 `LAPACK_ROW_MAJOR` / `LAPACK_COL_MAJOR`。

> **最容易踩的坑**：把行优先数组喂给默认列优先的接口，结果「矩阵被转置」。选对接口 + 传对 `layout` 是正确性第一关。

### 3.3 LP64 vs ILP64（整数接口层）

MKL 索引类型 `MKL_INT` 默认是 32 位 `int`（**LP64**）。当问题规模使元素数超过 `2^31`（如超大矩阵/长 FFT）时，需切到 **ILP64**（`MKL_INT` 变 64 位）：

```bash
# 编译期定义 + 链接 ILP64 接口库，或设环境变量 MKL_INTERFACE_LAYER=ILP64
clang++ -DMKL_ILP64 ... -L$MKLROOT/lib/intel64 -lmkl_intel_ilp64 -lmkl_sequential -lmkl_core
```

> 混用接口层（ILP64 库 + LP64 头，或反之）会在链接期报「找不到符号」或在超大数组时**静默截断索引**。全工程保持同一层。

---

## 4. 三种链接方式

### 4.1 一键（Intel 编译器，推荐上手）

```bash
icpx  -mkl            app.cpp   # 并行（Intel OpenMP）
icpx  -mkl=sequential app.cpp   # 串行，单线程
icpx  -mkl=cluster    app.cpp   # ScaLAPACK/BLACS，MPI 集群
```

### 4.2 动态运行时库（gcc/clang，最简）

```bash
clang++ -O2 app.cpp -I$MKLROOT/include -L$MKLROOT/lib -lmkl_rt -lm -o app
```

`-lmkl_rt` 单库含全部功能，运行期自动选内核，无需纠结线程库搭配。

### 4.3 静态三段式（精细控制时）

MKL 静态库按「**接口层 → 线程层 → 核心层**」三段链接，**顺序不能乱**：

```bash
# LP64 + Intel OpenMP 线程 + 核心，注意 -lmkl_* 后要跟 OpenMP 运行库
clang++ -fopenmp -O2 app.cpp -I$MKLROOT/include \
    -L$MKLROOT/lib \
    -lmkl_intel_lp64 -lmkl_intel_thread -lmkl_core \
    -lm -lpthread -ldl -o app
```

| 段 | 可选库（本机 `$MKLROOT/lib` 实测存在） | 作用 |
|----|----------------------------------------|------|
| 接口层 | `libmkl_intel_lp64` / `libmkl_intel_ilp64` | 索引位宽 + ABI |
| 线程层 | `libmkl_intel_thread` / `libmkl_gnu_thread` / `libmkl_sequential` | 并行后端（分别配 icx `-fiopenmp`、gcc `-fopenmp`、无） |
| 核心层 | `libmkl_core` | 计算内核本体 |

> 分布式另需 `libmkl_scalapack_lp64/ilp64` 与对应 BLACS 库，由 `-mkl=cluster` 自动串联。

---

## 5. 线程与并行控制

MKL 内部用 OpenMP 线程池自动并行（主要在 BLAS Level-3、FFT、大 VML）。控制手段三选一，优先级：**代码 API > 环境变量**。

```c
#include <mkl_service.h>
mkl_set_num_threads(16);          // 全局线程数
int n = mkl_get_max_threads();     // 查询当前上限

// 按「域」精细控制：BLAS / FFT / VML 分别给不同线程数
mkl_domain_set_num_threads(8,  MKL_DOMAIN_BLAS);   // MKL_DOMAIN_ALL=0, BLAS=1, FFT=2, VML=3
mkl_domain_set_num_threads(1,  MKL_DOMAIN_VML);
```

| 环境变量 | 含义 |
|----------|------|
| `MKL_NUM_THREADS=N` | 全局线程数 |
| `MKL_DOMAIN_NUM_THREADS="MKL_DOMAIN_BLAS=8; MKL_DOMAIN_FFT=4"` | 分域线程 |
| `MKL_DYNAMIC=TRUE/FALSE` | 是否允许运行期动态调线程 |

⚠️ **多线程 ≠ 更快**（本项目反复强调的测量纪律）：

- **嵌套并行会超订**：外层已开 OpenMP/MPI，内层 MKL 又各开一份线程 → 线程数爆炸、互相抢核。惯例是**外层并行时把 MKL 关成单线程**（`mkl_set_num_threads(1)` 或 `MKL_NUM_THREADS=1`），只让 MKL 在最内层算子做并行，二选一。
- **Intel 大小核混合架构**：MKL 盲信逻辑核数可能把线程派到能效核、或因频率不均反而变慢。多线程扩展性**必须本机实测**（详见 `10-MPI优化` / 测量纪律手册）。
- **小问题并行反而更慢**：矩阵太小时，线程创建/同步开销超过计算收益。

> 本项目的 MKL 示例统一 `mkl_set_num_threads(1)`，目的是**剥离并行干扰、只对比内核质量**，保证数字可复现。

---

## 6. 内存对齐与分配器

MKL 的 SIMD 内核期望**对齐数据**（现代 CPU 一般 64 字节最优）。非对齐访问可能导致向量流水线断裂甚至跨缓存行惩罚。

```c
#include <mkl_service.h>
void* p = mkl_malloc(1000 * sizeof(double), 64);  // 64 字节对齐分配
...
mkl_free(p);                                       // 必须用 mkl_free 释放
mkl_free_buffers();                                // 归还 MKL 内部缓冲池
```

- `mkl_malloc(size, alignment)` / `mkl_calloc(num, size, alignment)` / `mkl_realloc` / `mkl_free`：对齐分配接口，`alignment` 逐次显式指定（如 64）。
- `mkl_free_buffers()`：归还 MKL 内部缓冲池（长驻进程降峰用）。
- 用 `new`/`malloc` 的内存通常只按 8/16 字节对齐；大矩阵/长 FFT 建议改用 `mkl_malloc`。

> 经验：**大缓冲用 `mkl_malloc(.., 64)`**，其余交给 MKL 对齐要求（如 BLAS 的 `lda` 尽量取偶数/对齐倍数，避免跨行 cache 抖动）。

---

## 7. BLAS：稠密线性代数

三级层次，**Level-3 是算力主战场**：

| 级别 | 计算 | 例 | 算术强度 |
|------|------|-----|----------|
| L1 | 向量-向量 | `dot` `axpy` `asum` `scal` | O(1)，内存瓶颈 |
| L2 | 矩阵-向量 | `gemv` `symv` `trmv` | O(1)，内存瓶颈 |
| L3 | 矩阵-矩阵 | `gemm` `symm` `trsm` | O(N)，算力瓶颈 → 能吃满 GFLOPS |

`gemm` 计算 `C = alpha · op(A) · op(B) + beta · C`，`op` 可选不转置/转置/共轭转置。

```c
#include <mkl_cblas.h>
cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
            M, N, K, alpha, A, lda, B, ldb, beta, C, ldc);
```

- `lda/ldb/ldc`（leading dimension）= 该行优先矩阵的「主维度长度」（数组真实每行跨度），用于子矩阵与填充。
- 想复现 L3 高算术强度优势，务必让矩阵够大（如 ≥ 512³），小矩阵会退化成访存受限。

完整可跑对比（含 GFLOPS 与加速比）见 [`mkl_blas_dgemm.cpp`](../../basis/Compiler/MKL/mkl_blas_dgemm.cpp)。

---

## 8. VML：向量数学函数

VML 把标量数学函数「数组化」：一次调用处理整个数组，内部 SIMD 打包。

- 命名：`v` + 精度 + 函数 → `vdLn`（double 数组求自然对数）、`vsSin`（float 数组求 sin）。
- 调用形如 `vdLn(n, x, y)`：读 `x[n]`，写结果到 `y[n]`。
- **精度模式**用 `vmlSetMode()` 全局切换（返回旧模式，便于复位）：

| 模式 | 值 | 说明 |
|------|-----|------|
| 默认 | `0` | 高精度，结果几乎与 libm 一致 |
| `VML_HA` | 0x2 | 高精度 |
| `VML_LA` | 0x1 | 低精度，更快，误差约 ≤ 2⁻¹⁴ |
| `VML_EP` | 0x3 | 增强精度 |

```c
#include <mkl_vml_functions.h>
#include <mkl_vml_defines.h>
unsigned int old = vmlSetMode(VML_LA);  // 切低精度换吞吐
vdLn(n, x, y);
vmlSetMode(old);                        // 复位
```

> ⚠️ **VML 不是所有函数都稳赢 libm**：本项目实测 `vdLn`/`vdSqr` SIMD 收益明显（4~6x），但同一机器上 `vdSin`/`vdExp` 单线程下**反而慢于 glibc**——glibc 的 `exp/sin` 已高度优化，VML 对它们并无打包优势。**「用了 VML 就一定快」是错觉，性能以本机实测为准。**

示例见 [`mkl_vml_log.cpp`](../../basis/Compiler/MKL/mkl_vml_log.cpp)（含默认/低精度两档的加速比与最大误差）。

---

## 9. LAPACK：高阶数值算法

LAPACK 构建在 BLAS 之上，提供矩阵分解与求解。用 `LAPACKE_*` C 接口可指定行/列优先。

| 任务 | 例程 | 说明 |
|------|------|------|
| LU 分解 / 解方程 | `getrf` / `getrs` / `gesv` | `gesv` = 分解+回代一步到位 |
| Cholesky | `potrf` / `posv` | 对称正定矩阵更快 |
| QR | `geqrf` / `orgqr` | 最小二乘、特征值前置 |
| 特征值 | `syev`（对称）/ `geev`（一般） | |
| SVD | `gesdd` / `gesvd` | |

```c
#include <mkl_lapacke.h>
lapack_int info = LAPACKE_dgesv(LAPACK_ROW_MAJOR, n, nrhs,
                                A, lda, ipiv, B, ldb);  // A 被覆写为 LU，B 被覆写为解 X
```

- 返回 `info`：`0` 成功；`<0` 第 `|info|` 个参数非法；`>0` 表示 U 对角元为 0（矩阵奇异，无法继续）。
- `ipiv` 是主元索引数组（长度 `n`），调用前分配即可，内部使用。
- **接口会就地覆盖输入**：`A`→LU 因子、`B`→解。要保留原矩阵/右端项需先拷贝。

可跑示例（解 5×5 并核对残差）见 [`mkl_lapack_dgesv.cpp`](../../basis/Compiler/MKL/mkl_lapack_dgesv.cpp)。

---

## 10. FFT / DFTI：离散傅里叶变换

DFTI（Discrete Fourier Transform Interface）走「**描述符三步曲**」：

```c
#include <mkl_dfti.h>
DFTI_DESCRIPTOR_HANDLE desc = NULL;
DftiCreateDescriptor(&desc, DFTI_DOUBLE, DFTI_COMPLEX, 1, n);   // 精度/域/维度/长度
DftiSetValue(desc, DFTI_PLACEMENT, DFTI_NOT_INPLACE);           // 输入输出分离
DftiCommitDescriptor(desc);                                      // 预计算 twiddle、选内核
DftiComputeForward(desc, x_in, x_out);                           // 执行
DftiFreeDescriptor(&desc);
```

- **域**：`DFTI_COMPLEX`（复数↔复数）、`DFTI_REAL`（实数↔共轭对称的 Hermitian 输出）。
- **复数布局**：默认交织存储 `re0,im0,re1,im1,…`，`n` 个复数需长度 `2n` 的实数数组。
- **归一化**：MKL 默认 `FORWARD_SCALE=1`、`BACKWARD_SCALE=1`，即 IFFT(FFT(x)) = `n·x`。想要「无损还原」设 `DFTI_BACKWARD_SCALE = 1.0/n`。
- **描述符可复用**：一次 `Commit`，可对多组数据反复 `Compute`——把建立开销摊薄。
- 长度选 **2 的幂或光滑数**（因子只含 2/3/5/7）内核最快；大素数长度性能显著下降。

可跑示例（一维复数 FFT 正/逆变换 + round-trip 校验）见 [`mkl_fft_dfti.cpp`](../../basis/Compiler/MKL/mkl_fft_dfti.cpp)。

---

## 11. 环境变量速查

| 变量 | 作用 |
|------|------|
| `MKLROOT` | MKL 安装根目录（头/库路径来源） |
| `MKL_NUM_THREADS` | 全局线程数 |
| `MKL_DOMAIN_NUM_THREADS` | 分域线程（BLAS/FFT/VML…） |
| `MKL_DYNAMIC` | 是否动态调整线程 |
| `MKL_INTERFACE_LAYER` | `LP64` / `ILP64` 接口层 |
| `MKL_ENABLE_INSTRUCTIONS` | 限定最高指令集：`SSE2`/`AVX`/`AVX2`/`AVX512`（代码：`mkl_enable_instructions(MKL_ENABLE_AVX512)`） |
| `MKL_VERBOSE` / `ONEAPI_MKL_VERBOSE` | 打印每次 MKL 调用与所用内核派发，定位「到底调了没调 MKL」 |
| `MKL_CBWR` | 条件数无关位复现（跨指令集比特级一致结果） |

> 排查「加速没生效」时，先 `MKL_VERBOSE=1` 跑一遍，确认调用确实进了 MKL、并看它派发了哪个内核。

---

## 12. 调优清单与常见陷阱

- ✅ **选对接口层**：行优先用 `cblas_*` / `LAPACKE_*`；别把行优先数据喂给列优先例程。
- ✅ **数据要够大**：BLAS Level-3、VML、FFT 都有并行/打包阈值，太小则开销盖过收益。
- ✅ **大缓冲用 `mkl_malloc(.., 64)`**，满足 SIMD 对齐。
- ✅ **嵌套并行时 MKL 关成单线程**，避免超订；或反过来只用 MKL 内层并行。
- ✅ **多线程必须实测**：混合架构、NUMA、核数上限都会让「理论加速」落空。
- ✅ **精度换性能用 VML `VML_LA`**，并核对误差是否可接受。
- ⚠️ **别假设「MKL 一定更快」**：个别 VML 函数（`exp/sin`）单线程下可能输给 glibc；用数字说话。
- ⚠️ **接口层混用**（ILP64 库 + LP64 头）会链接报错或大数组静默截断。
- ⚠️ **LAPACK 就地覆盖输入**，需保留原矩阵先自行拷贝。
- ⚠️ **FFT 归一化默认不正则化**，还原要显式设 `BACKWARD_SCALE`。

---

## 13. 示例代码与延伸阅读

**配套可跑示例**（四大支柱，均含详细中文注释 + 编译命令 + 本机实测）：

| 文件 | 演示 |
|------|------|
| [`basis/Compiler/MKL/mkl_blas_dgemm.cpp`](../../basis/Compiler/MKL/mkl_blas_dgemm.cpp) | BLAS 矩阵乘：手写 vs `cblas_dgemm`（~5x） |
| [`basis/Compiler/MKL/mkl_vml_log.cpp`](../../basis/Compiler/MKL/mkl_vml_log.cpp) | VML 数组 `ln`：精度模式与 SIMD 打包 |
| [`basis/Compiler/MKL/mkl_lapack_dgesv.cpp`](../../basis/Compiler/MKL/mkl_lapack_dgesv.cpp) | LAPACK 解线性方程组 + 残差核对 |
| [`basis/Compiler/MKL/mkl_fft_dfti.cpp`](../../basis/Compiler/MKL/mkl_fft_dfti.cpp) | FFT/DFTI 一维变换 + round-trip |
| [`basis/Compiler/MKL/README.md`](../../basis/Compiler/MKL/README.md) | 环境准备、两种链接、实测输出汇总 |

**项目内关联**：

- 课程笔记：[`04-编译器概述.md`](../notes/04-编译器概述.md)（数学库简提 + 向量化）
- 理论长文：[`compiler.md`](compiler.md)（BLAS / MKL / `dgemm` 命名一节）
- 测量纪律：[`../notes/30-测量纪律手册.md`](../notes/30-测量纪律手册.md)（多线程实测、防死代码删除）
- 相关基线：[`basis/Compiler/math_mkl.cpp`](../../basis/Compiler/math_mkl.cpp)（`cblas_dzasum`）、[`basis/Compiler/cBLAS/`](../../basis/Compiler/cBLAS/)（参考 CBLAS 实现）

**外部参考**：Intel oneMKL 官方文档（`https://www.intel.com/content/www/us/en/developer/tools/oneapi/onemkl.html`）与《oneMKL Reference》《User Guide for LAPACK-95 / C / Fortran Interfaces》。
