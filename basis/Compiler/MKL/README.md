# Intel MKL 使用示例 (Math Kernel Library)

**Intel MKL（Math Kernel Library）** 是 Intel 提供的高性能数学库，针对 Intel 处理器
做了分块 + SIMD + FMA + 多线程的深度优化，覆盖 BLAS / LAPACK / VML / FFT 等标准接口。
在性能敏感的数值计算里，**优先调用 MKL 打包内核，通常远快于自己写循环**——这正是
[`docs/notes/04-编译器概述.md`](../../../docs/notes/04-编译器概述.md) 「数学库」一节强调的要点。

本目录按 MKL 的四大功能支柱各给一个**可编译、可运行、可核对**的最小示例。

---

## 一、示例清单

| 文件 | MKL 支柱 | 演示内容 | 关键 API |
|------|----------|----------|----------|
| [`mkl_blas_dgemm.cpp`](mkl_blas_dgemm.cpp) | **BLAS**（线性代数） | Level-3 矩阵乘：手写三重循环 vs MKL 打包内核 | `cblas_dgemm` |
| [`mkl_vml_log.cpp`](mkl_vml_log.cpp) | **VML**（向量数学） | 数组级 `ln`：标量 `log()` 循环 vs SIMD 打包，及精度模式权衡 | `vdLn` / `vmlSetMode` |
| [`mkl_lapack_dgesv.cpp`](mkl_lapack_dgesv.cpp) | **LAPACK**（高阶数值） | LU 分解解线性方程组 `A x = b`，核对残差 | `LAPACKE_dgesv` |
| [`mkl_fft_dfti.cpp`](mkl_fft_dfti.cpp) | **FFT / DFTI**（变换） | 一维复数 FFT 正/逆变换，验证 round-trip 还原 | `DftiCreateDescriptor` / `DftiComputeForward` |

> 各文件头部都有详细的中文注释：概念、命名规则、函数签名逐参数解释、编译命令。

---

## 二、环境准备

MKL 随 Intel oneAPI HPC Toolkit 安装。运行前先设置环境变量：

```bash
source /opt/intel/oneapi/setvars.sh          # 设置 MKLROOT、LD_LIBRARY_PATH 等
echo $MKLROOT                                 # 例如 /opt/intel/oneapi/mkl/2026.1
```

---

## 三、编译方式

任选其一（`<name>` 替换为上面四个文件名之一）：

```bash
# 方式 1：clang / gcc + 手动链接 MKL 运行时库（推荐，直观）
clang++ -O2 <name>.cpp -I$MKLROOT/include -L$MKLROOT/lib -lmkl_rt -lm -o <name>

# 方式 2：Intel 编译器一键链接 MKL
icpx -mkl <name>.cpp -o <name>            # -mkl = 并行链接；-mkl=sequential 串行

# 运行（若未 source setvars.sh，需手动指定动态库路径）
LD_LIBRARY_PATH=$MKLROOT/lib ./<name>
```

选项解释：

| 选项 | 含义 |
|------|------|
| `-I$MKLROOT/include` | MKL 头文件路径（`mkl_cblas.h` / `mkl_vml_functions.h` / `mkl_lapacke.h` / `mkl_dfti.h`） |
| `-L$MKLROOT/lib` | MKL 库文件搜索路径 |
| `-lmkl_rt` | 链接 **MKL 运行时库**（动态调度版，一个库含 BLAS/LAPACK/VML/FFT） |
| `-lm` | 链接 libm，供参考用的标量 `log()`/`sin()` 等 |
| `-mkl` | icpx/icc 专用：自动完成上面所有 MKL 链接 |

---

## 四、本机实测输出（Intel oneAPI MKL 2026.1，单线程）

> 为剥离多线程干扰、结果可复现，示例均 `mkl_set_num_threads(1)`，只比「内核质量」。

**BLAS — 矩阵乘法（N=1000）**
```
naive_gemm        : 0.1466 s   (13.64 GFLOPS)
cblas_dgemm (MKL) : 0.0288 s   (69.43 GFLOPS)
speedup           : 5.1x        ← 一个字没改循环写法，交给 MKL 就快 5 倍
```

**VML — 数组 ln（N=2×10⁷）**
```
scalar loop log() : 0.1132 s
vdLn  (default)   : 0.0277 s   speedup=4.08x   max_err=0.00e+00    ← SIMD 打包，且完全精确
vdLn  (VML_LA)    : 0.0193 s   speedup=5.87x   max_err=8.88e-16    ← 低精度模式再快一档
```

**LAPACK — 解 5×5 方程组**
```
x = [1, 2, 3, 4, 5]（与已知真解一致）
max relative error : 2.22e-16      residual ||A x - b|| : 1.74e-14
```

**FFT / DFTI — 一维复数 FFT（N=8）**
```
幅度谱 |X| 关于 Nyquist 对称（实信号特性）
round-trip IFFT(FFT(x)) max abs error : 4.44e-16    ← 正变换后能无损还原
```

---

## 五、几点提醒（对齐工程「测量纪律」）

1. **多线程不等于更快**：Intel 大小核混合架构上，MKL 盲信核数可能因超线程 / 频率不均而变慢。
   需要并行时，改 `mkl_set_num_threads(n)` 或设 `MKL_NUM_THREADS`，**并实测确认**。
2. **VML 并非所有函数都稳赢 libm**：`ln/sqr` 这类 SIMD 收益明显；个别函数（如某些机器上的
   `exp/sin`）glibc 已极快，单线程下 MKL 未必占优。示例演示的是**用法**，性能以本机实测为准。
3. **优先让 MKL / 编译器干活**：能用 `cblas_dgemm` 就别手写三重循环；能用 `-O3 -march=native`
   自动向量化就别锁死窄版本 intrinsics（见 `04-编译器概述.md` 的宽度锁定结论）。

---

## 六、延伸阅读

- **独立理论文档**：[`docs/theory/mkl.md`](../../../docs/theory/mkl.md)（MKL 全景参考—组件/接口约定/链接方式/线程对齐/调优清单）
- 理论长文：[`docs/theory/compiler.md`](../../../docs/theory/compiler.md)（BLAS / MKL / dgemm 命名一节）
- 课程笔记：[`docs/notes/04-编译器概述.md`](../../../docs/notes/04-编译器概述.md)（数学库简提 + 向量化）
- 同类代码：[`../math_mkl.cpp`](../math_mkl.cpp)（`cblas_dzasum`）、[`../cBLAS/`](../cBLAS/)（参考 CBLAS 实现）
