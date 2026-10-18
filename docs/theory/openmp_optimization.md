# OpenMP 程序优化


- [OpenMP 程序优化](#openmp-程序优化)
  - [1. OpenMP 编程简介](#1-openmp-编程简介)
    - [1.1 什么是 OpenMP](#11-什么是-openmp)
    - [1.2 并行执行模式](#12-并行执行模式)
    - [1.3 OpenMP 程序组成](#13-openmp-程序组成)
    - [1.4 指导语句语法](#14-指导语句语法)
    - [1.5 parallel 和 for 指导语句](#15-parallel-和-for-指导语句)
    - [1.6 示例：OpenMP 版矩阵乘法](#16-示例openmp-版矩阵乘法)
  - [2. 并行区重构](#2-并行区重构)
    - [2.1 概述](#21-概述)
    - [2.2 并行区扩张](#22-并行区扩张)
      - [循环结构并行区扩张](#循环结构并行区扩张)
      - [函数结构并行区扩张](#函数结构并行区扩张)
    - [2.3 并行区合并](#23-并行区合并)
      - [变量属性冲突处理](#变量属性冲突处理)
      - [串行语句处理](#串行语句处理)
  - [3. 避免伪共享](#3-避免伪共享)
    - [3.1 什么是伪共享](#31-什么是伪共享)
    - [3.2 数据填充避免伪共享](#32-数据填充避免伪共享)
    - [3.3 数据私有避免伪共享](#33-数据私有避免伪共享)
  - [4. 循环向量化](#4-循环向量化)
    - [4.1 向量化指导命令](#41-向量化指导命令)
    - [4.2 语法](#42-语法)
    - [4.3 示例](#43-示例)
  - [5. 负载均衡优化](#5-负载均衡优化)
    - [5.1 循环嵌套合并调度（collapse）](#51-循环嵌套合并调度collapse)
    - [5.2 线程调度策略](#52-线程调度策略)
  - [6. 线程数设置优化](#6-线程数设置优化)
    - [6.1 串并行切换](#61-串并行切换)
    - [6.2 线程数设置模式](#62-线程数设置模式)
  - [7. 避免隐式同步](#7-避免隐式同步)
    - [7.1 分析隐式同步](#71-分析隐式同步)
    - [7.2 消除隐式同步](#72-消除隐式同步)
  - [8. 流水并行优化](#8-流水并行优化)
    - [8.1 DOALL 与 DOACROSS 循环](#81-doall-与-doacross-循环)
    - [8.2 流水并行步骤](#82-流水并行步骤)
    - [8.3 同步实现](#83-同步实现)
    - [8.4 流水并行粒度](#84-流水并行粒度)
      - [循环分块增大粒度](#循环分块增大粒度)
      - [循环分段减小粒度](#循环分段减小粒度)
      - [循环分段 + 循环交换减小粒度](#循环分段--循环交换减小粒度)
    - [8.5 分块大小选择](#85-分块大小选择)
  - [9. 访存优化](#9-访存优化)
    - [9.1 NUMA 感知内存分配](#91-numa-感知内存分配)
    - [9.2 数据布局优化](#92-数据布局优化)
    - [9.3 线程亲和性设置](#93-线程亲和性设置)
  - [总结](#总结)

---

## 1. OpenMP 编程简介

### 1.1 什么是 OpenMP

OpenMP 是一种用于共享内存并行编程的多线程程序设计方案，适合在多核系统上进行并行程序设计。OpenMP 降低了多核并行编程的难度，使优化人员可以更多地考虑算法本身，而非具体的并行实现细节。

**版本演进：**

| 年份 | 版本        | 主要特性                       |
| ---- | ----------- | ------------------------------ |
| 1997 | C/C++ 1.0   | 初始规范                       |
| 1998 | Fortran 1.0 | 初始规范                       |
| 2000 | C/C++ 2.0   | -                              |
| 2002 | Fortran 2.0 | -                              |
| 2005 | OpenMP 2.5  | 融合 C/C++ 和 Fortran 规范     |
| 2008 | OpenMP 3.0  | 添加 task                      |
| 2013 | OpenMP 4.0  | 添加向量化，支持 GPU/异构设备  |
| 2015 | OpenMP 4.5  | 添加 taskloop，更好的 GPU 支持 |
| 2018 | OpenMP 5.0  | tool interface                 |
| 2020 | OpenMP 5.1  | -                              |
| 2021 | OpenMP 5.2  | -                              |

### 1.2 并行执行模式

OpenMP 支持两种并行执行模式：

- **Fork-Join 模式**：在并行区的开始位置创建多个线程，在并行区的结束位置自动合并线程。串行代码由主线程执行，并行区由多个线程并行执行，并行区结束处进行栅障同步。
- **SPMD 模式**：在单个并行程序中按照线程号来匹配程序分支，不同线程分配不同的计算任务。

### 1.3 OpenMP 程序组成

OpenMP 程序由三部分组成：

- **编译指导语句**：串行程序实现并行化的桥梁，是编写 OpenMP 程序的关键。
- **库函数**：在程序运行阶段改变和优化并行环境，从而控制程序的运行。
- **环境变量**：库函数中控制函数运行的具体参数。

### 1.4 指导语句语法

```c
#pragma omp parallel for num_threads(4) private(tid, mcpu) \
    shared(sum)
for (int i = 0; i < N; i++) {
    // 并行执行代码
}
```

指导语句由以下部分组成：指导标识符 → 指导命令 → 子句列表 → 续行符/换行符。

**指导命令分类：**

| 类别           | 命令                                                          |
| -------------- | ------------------------------------------------------------- |
| 并行控制类     | `parallel`、`simd`                                            |
| 工作共享类     | `for`、`sections`、`task`、`single`、`target`                 |
| 线程同步类     | `barrier`、`master`、`critical`、`atomic`、`flush`、`ordered` |
| 复合指导命令类 | `parallel for`、`for simd`、`parallel sections`               |

### 1.5 parallel 和 for 指导语句

**`#pragma omp parallel` 子句：**

- `allocate([allocator:] list)`
- `copyin(list)`
- `default(shared | firstprivate | private | none)`
- `firstprivate(list)`
- `if([parallel:] omp-logical-expression)`
- `num_threads(nthreads)`
- `private(list)`
- `proc_bind(close | primary | spread)`
- `reduction([reduction-modifier,] reduction-identifier: list)`
- `shared(list)`

**`#pragma omp for` 子句：**

- `allocate([allocator:] list)`
- `collapse(n)`
- `firstprivate(list)`
- `lastprivate([lastprivate-modifier:] list)`
- `linear(list[: linear-step])`
- `nowait`
- `order([order-modifier:] concurrent)`
- `ordered[(n)]`
- `private(list)`
- `reduction([reduction-modifier,] reduction-identifier: list)`
- `schedule([modifier [, modifier]:] kind[, chunk_size])`

### 1.6 示例：OpenMP 版矩阵乘法

```c
#include <stdio.h>
#include <omp.h>
#define N 2000

float A[N][N], B[N][N], C[N][N];

int main() {
    int i, j, k;
    float sum = 0.0;
    double start_time, end_time, used_time;

    // 初始化
    for (i = 0; i < N; i++) {
        for (j = 0; j < N; j++) {
            A[i][j] = i + 1.0;
            B[i][j] = 1.0;
            C[i][j] = 0.0;
        }
    }

    // 并行化改写：对外层循环 i 进行并行化
    start_time = omp_get_wtime();
    #pragma omp parallel for private(j, k) shared(A, B, C) num_threads(4)
    for (i = 0; i < N; i++)
        for (j = 0; j < N; j++)
            for (k = 0; k < N; k++)
                C[i][j] += A[i][k] * B[k][j];
    end_time = omp_get_wtime();

    // 求和验证
    for (i = 0; i < N; i++)
        for (j = 0; j < N; j++)
            sum += C[i][j];

    used_time = end_time - start_time;
    printf("sum=%lf, used_time=%lf s\n", sum, used_time);
}
```

**编译与运行：**

```shell
# 编译（-O2 启用优化，-fopenmp 启用 OpenMP 支持）
gcc -O2 -fopenmp openmp_matmul.c -o openmp_matmul

# 运行（默认线程数由 OMP_NUM_THREADS 环境变量控制）
export OMP_NUM_THREADS=8
./openmp_matmul
# 输出示例：sum=8000400000.000000, used_time=2.345678 s
```

> **注意**：对 `for` 循环进行并行化时，应选择合适的循环层级。对最外层循环 `i` 并行化（✓），对内层循环 `j` 或 `k` 并行化（×）会导致线程创建/合并过于频繁。

> **性能提示**：矩阵乘法的并行化应选择最外层循环，因为其迭代次数最大（N 次），能充分利用多线程并行。同时应注意矩阵的数据布局——C 语言采用行优先存储，内层循环 `k` 的连续访问 `A[i][k]` 具有良好的空间局部性，而 `B[k][j]` 的跨列访问则可能产生缓存未命中。

---

## 2. 并行区重构

### 2.1 概述

OpenMP Fork-Join 模式下，线程的创建和合并比较频繁，并行性表达处于低效状态。

**并行区重构**是结合数据和计算划分等信息，通过改变原并行区的结构，降低串并行程序之间切换以及其他开销。包括两种方式：

- **并行区扩张**：将并行区范围扩大
- **并行区合并**：将多个并行区合并为一个

### 2.2 并行区扩张

#### 循环结构并行区扩张

针对包含整个循环结构的并行区，将并行区扩张到循环之外。若循环迭代次数为 N，则并行区扩张后线程的创建和合并次数将减少 N-1 次。

**优化前：** 每次循环迭代都创建/合并线程

```c
for (i = 0; i < N; i++)
    #pragma omp parallel for private(j, k) shared(A, B, C) num_threads(4)
    for (j = 0; j < N; j++)
        for (k = 0; k < N; k++)
            C[i][j] += A[i][k] * B[k][j];
```

**优化后：** 并行区扩张到外层

```c
#pragma omp parallel for private(j, k) shared(A, B, C) num_threads(4)
for (i = 0; i < N; i++)
    for (j = 0; j < N; j++)
        for (k = 0; k < N; k++)
            C[i][j] += A[i][k] * B[k][j];
```

> **结论**：并行区扩张后，线程的创建和合并次数从 N 次降为 1 次，消除了 N-1 次 fork-join 开销。对于迭代次数较大的循环，这一优化可带来显著的性能提升。

#### 函数结构并行区扩张

将函数结构内部的并行区扩张到整个函数结构外部，使函数内部全部语句包含在并行区中，进一步获得更多并行区合并机会。

**优化前：**

```c
void init_array(int* a) {
    #pragma omp for
    for (int i = 0; i < N; i++)
        a[i] = i;
}

int main() {
    init_array(A);
    init_array(B);
    init_array(C);
}
```

**优化后：**

```c
void init_array(int* a) {
    for (int i = 0; i < N; i++)
        a[i] = i;
}

int main() {
    #pragma omp parallel {
        init_array(A);
        init_array(B);
        init_array(C);
    }
}
```

### 2.3 并行区合并

并行区合并不仅仅是将多个并行区改写至一个 `#pragma omp parallel` 区域，还需要保证合并后各个子线程间的数据更新顺序和执行顺序与原程序保持一致：

- **数据更新顺序**：通过 `flush` 指导语句实现
- **执行顺序**：通过 `barrier` 指导语句进行同步

此外还需考虑变量数据属性冲突以及并行区之间串行语句的处理等问题。

#### 变量属性冲突处理

当两个并行区对同一变量的数据属性不同时，合并后需要统一属性。例如原程序中第一个并行区将变量 `k` 声明为 `shared`，第二个并行区声明为 `firstprivate`，合并后应统一使用 `firstprivate`。

**优化前：**

```c
int main() {
    int k = 2, sum1 = 0, sum2 = 0;
    #pragma omp parallel shared(k) {
        #pragma omp for reduction(+: sum1)
        for (int i = 0; i < 10000; i++)
            sum1 += (k + i);
    }
    #pragma omp parallel firstprivate(k) {
        #pragma omp for reduction(+: sum2)
        for (int j = 0; j < 10000; j++)
            sum2 += (2 * k + j);
    }
}
```

**优化后：**

```c
int main() {
    int k = 2, sum1 = 0, sum2 = 0;
    #pragma omp parallel firstprivate(k) shared(sum1, sum2) {
        #pragma omp for reduction(+: sum1)
        for (int i = 0; i < 10000; i++)
            sum1 += (k + i);
        #pragma omp single {
            sum2 = sum1;
            k++;
        }
        #pragma omp for reduction(+: sum2)
        for (int j = 0; j < 10000; j++)
            sum2 += (2 * k + j);
    }
}
```

#### 串行语句处理

并行区之间的串行语句可以使用 `#pragma omp master` 或 `#pragma omp single` 包裹，确保只执行一次。

> **结论**：并行区合并的关键在于保证数据更新顺序和执行顺序的正确性。合并后减少了并行区数量，降低了线程创建/合并和同步的总开销。实际优化中，通常先进行并行区扩张，再进行并行区合并，两者配合使用效果最佳。

---

## 3. 避免伪共享

### 3.1 什么是伪共享

OpenMP 在多核处理器间进行同步时常常需要共享变量。当多个线程对同一个数组进行修改时，即使线程间从算法上并不需要共享变量，但若不同线程需要赋值的地址处于同一个缓存行中（通常 64 字节），就会引起缓存冲突，严重降低程序性能。这就是**伪共享**（False Sharing）。

**示例：** 数组 `result[Nthreads][8]`，`result[0]` 大小为 8Byte × 8 = 64Byte，恰好等于一个缓存行。线程 0 和线程 2 同时写 `result[0]` 的不同元素，会导致缓存行在两个核心间反复失效。

> **查看缓存行大小**：可通过系统命令查看当前平台的缓存行大小，以便进行正确的数据填充。
> ```shell
> getconf LEVEL1_DCACHE_LINESIZE
> # 输出示例：64
> ```

### 3.2 数据填充避免伪共享

通过增加数组维度，使每个线程的数据占据独立的缓存行。

**优化前（存在伪共享）：**

```c
double result[Nthreads] = {0.0};

#pragma omp parallel num_threads(Nthreads) {
    int id = omp_get_thread_num();
    double temp;
    #pragma omp for
    for (i = 0; i < num_steps; i++) {
        temp = (i + 0.5) * step;
        result[id] += 4.0 / (1.0 + temp * temp);
    }
}
for (i = 0; i < Nthreads; i++)
    pi += result[i];
pi = step * pi;
```

**优化后（数据填充）：**

```c
double result[Nthreads][8] = {0.0};  // 每个线程独占一个缓存行

#pragma omp parallel num_threads(Nthreads) {
    int id = omp_get_thread_num();
    double temp;
    #pragma omp for
    for (i = 0; i < num_steps; i++) {
        temp = (i + 0.5) * step;
        result[id][0] += 4.0 / (1.0 + temp * temp);
    }
}
for (i = 0; i < Nthreads; i++)
    pi += result[i][0];
pi = step * pi;
```

### 3.3 数据私有避免伪共享

使用 `reduction` 子句让每个线程拥有变量的私有副本，最后自动归约合并。

```c
double result = 0.0;

#pragma omp parallel num_threads(Nthreads) {
    double temp;
    #pragma omp for reduction(+: result)
    for (i = 0; i < num_steps; i++) {
        temp = (i + 0.5) * step;
        result += 4.0 / (1.0 + temp * temp);
    }
}
pi = step * result;
```

> **归约操作**（reduction）是指反复将运算符作用在一个变量上并保存结果。`reduction` 子句对前后有依赖的循环进行归约并行化：每个线程创建变量的私有副本并初始化，最终将各线程的私有副本通过指定操作符合并。
>
> **支持的归约操作符**：`+`、`*`、`-`、`&`、`|`、`^`、`&&`、`||`，以及 `min`、`max`（OpenMP 5.0+）。

> **结论**：避免伪共享的两种方法各有适用场景——**数据填充**适用于无法使用归约操作的场景（如复杂的写入模式），但会增加内存占用；**reduction 子句**适用于归约操作（如求和、求最大值），代码简洁且无需额外内存开销。优先考虑使用 `reduction` 子句。

---

## 4. 循环向量化

### 4.1 向量化指导命令

OpenMP 中有两种支持向量化的语句：

- **`#pragma omp simd`**：对循环进行单线程的数据级并行（向量化）
- **`#pragma omp for simd`**：结合 `for` 和 `simd`，既将迭代分配给各线程（并行化），又对每个线程的计算任务进行向量化

### 4.2 语法

**`#pragma omp simd` 子句：**

`aligned(list[: alignment])`、`collapse(n)`、`if([simd:] omp-logical-expression)`、`lastprivate([lastprivate-modifier:] list)`、`linear(list[: linear-step])`、`nontemporal(list)`、`order([order-modifier:] concurrent)`、`private(list)`、`reduction(...)`、`safelen(length)`、`simdlen(length)`

**`#pragma omp for simd` 子句：**

在 `simd` 子句基础上增加：`firstprivate(list)`、`nowait`、`ordered[(n)]`、`schedule([modifier [, modifier]:] kind[, chunk_size])`

### 4.3 示例

**纯向量化：**

```c
#pragma omp simd
for (int i = 0; i < N; i++)
    for (int j = 0; j < N; j++)
        for (int k = 0; k < N; k++)
            C[i][j] += A[i][k] * B[k][j];
```

**并行化 + 向量化：**

```c
#pragma omp parallel num_threads(4)
#pragma omp for simd simdlen(8)
for (int i = 0; i < N; i++)
    for (int j = 0; j < N; j++)
        for (int k = 0; k < N; k++)
            C[i][j] += A[i][k] * B[k][j];
```

**编译与向量化验证：**

```shell
# 使用 -O2 或 -O3 启用自动向量化
gcc -O3 -fopenmp program.c -o program

# 显式启用 OpenMP SIMD（独立于线程并行）
gcc -O3 -fopenmp-simd program.c -o program

# 查看编译器向量化报告（GCC）
gcc -O3 -fopenmp -fopt-info-vec-optimized program.c
# 输出示例：program.c:15:5: note: loop vectorized

# 查看编译器向量化报告（Clang）
clang -O3 -fopenmp -Rpass=loop-vectorize program.c
```

> **结论**：`simd` 指导命令是对编译器的显式提示，配合 `-O2`/`-O3` 优化级别可确保向量化生效。使用编译器报告可验证循环是否成功向量化，以及未向量化的原因（如存在依赖、对齐问题等）。

---

## 5. 负载均衡优化

OpenMP 程序中影响负载均衡的因素包括：线程创建/回收开销、线程调度开销、线程同步开销。

### 5.1 循环嵌套合并调度（collapse）

`collapse(n)` 子句将最相邻的 `n` 层循环的迭代压缩合并为更大的任务调度空间，增加可调度迭代次数，有助于解决负载不均衡问题。

```c
#pragma omp parallel for private(i, j, k) shared(A, B, C) num_threads(4) collapse(2)
for (i = 0; i < N; i++)
    for (j = 0; j < N; j++)
        for (k = 0; k < N; k++)
            C[i][j] += A[i][k] * B[k][j];
```

### 5.2 线程调度策略

选择合适的调度策略，使各个线程的工作量相当：

```c
#pragma omp for schedule(schedule_name, chunk_size)
```

| 调度策略  | 说明                                                     | 适用场景                             |
| --------- | -------------------------------------------------------- | ------------------------------------ |
| `static`  | 将循环迭代划分为大小相等的调度块，迭代在线程上尽可能均分 | 每次迭代计算量相近的规则循环         |
| `dynamic` | 使用"先来先服务"策略，线程执行完当前块后从队列中分配新块 | 每次迭代计算量差异较大的不规则循环   |
| `guided`  | 调度块大小动态变化，开始较大，按指数关系逐渐变小         | 计算量从头到尾递减的循环             |
| `runtime` | 运行时使用环境变量 `OMP_SCHEDULE` 确定调度策略           | 需要在不重新编译的情况下调整调度策略 |

**调度策略选择建议：**

| 循环结构类型           | 推荐策略                                |
| ---------------------- | --------------------------------------- |
| 规则循环（如矩阵乘法） | 带参数的 `static` 调度                  |
| 递增型循环             | 带参数的 `static` 调度                  |
| 递减型循环             | 优先 `guided`，若负载不均则用 `dynamic` |
| 随机型循环             | 优先 `dynamic` 调度                     |

> **结论**：调度策略的选择本质上是在**负载均衡**和**调度开销**之间寻找平衡。`static` 调度开销最低但假设各迭代工作量相等；`dynamic` 和 `guided` 通过运行时动态分配实现更好的均衡，但引入额外的同步开销。对于大多数规则循环，默认的 `static` 调度即可满足需求。

---

## 6. 线程数设置优化

### 6.1 串并行切换

OpenMP 提供 `if` 子句来自动切换程序的并行和串行执行。当计算量 N 大于阈值 Number 时并行执行，否则串行执行。阈值需要根据运行环境和线程数不断调整测试来确定。

```c
#pragma omp parallel for private(i, j, k) shared(A, B, C) num_threads(8) if(N > 86)
for (i = 0; i < N; i++)
    for (j = 0; j < N; j++)
        for (k = 0; k < N; k++)
            C[i][j] += A[i][k] * B[k][j];
```

### 6.2 线程数设置模式

| 模式         | 方法                                                                       |
| ------------ | -------------------------------------------------------------------------- |
| **静态模式** | `omp_set_num_threads()`、子句 `num_threads`、环境变量 `OMP_NUM_THREADS`    |
| **动态模式** | 采用默认模式（不指定线程数），`omp_set_dynamic()` 设定并行区内线程数目上限 |
| **嵌套模式** | `omp_set_nested()` 启用或禁用嵌套并行（并行区中构建另一个并行区）          |
| **条件模式** | 利用 `if` 子句切换串行/并行模式                                            |

```c
omp_set_dynamic(1);
omp_set_num_threads(8);
#pragma omp parallel for private(i, j, k) shared(A, B, C) if(N > 86)
for (i = 0; i < N; i++)
    for (j = 0; j < N; j++)
        for (k = 0; k < N; k++)
            C[i][j] += A[i][k] * B[k][j];
```

**环境变量与查询函数：**

```shell
# 通过环境变量设置线程数
export OMP_NUM_THREADS=8

# 通过环境变量动态调整线程数上限
export OMP_DYNAMIC=true
```

```c
// 查询当前并行区的线程数
int max_threads = omp_get_max_threads();

// 查询系统可用的处理器/核心数
int num_procs = omp_get_num_procs();

// 查询当前线程在并行区中的编号
int thread_id = omp_get_thread_num();
```

> **注意**：增加线程可以在特定时间内完成更多任务，但线程数量过多会导致同步等开销增加，反而降低性能。不同程序的最优线程数不同，需根据程序特征和运行环境来确定。一般建议线程数不超过物理核心数（而非逻辑核心数），以避免超线程带来的资源竞争。

---

## 7. 避免隐式同步

### 7.1 分析隐式同步

OpenMP 有显式和隐式两种同步方式：

- **显式同步**：使用 `#pragma omp barrier`，要求并行区内所有线程都执行到该处才能继续。
- **隐式同步**：执行 `parallel`、`for`、`sections` 等指导语句时，并行代码结束处会自动添加同步点。

```c
int i, A[N];
#pragma omp parallel private(i) num_threads(4) {
    #pragma omp for
    for (i = 0; i < N; i++) {
        A[i] = i;
    }
    // 此处存在隐式 barrier
    #pragma omp master
    printf("All work done\n");
}
```

### 7.2 消除隐式同步

当后续任务不需要等待前面任务完成时，可使用 `nowait` 子句消除隐式同步，让先完成的线程继续工作。

```c
#pragma omp parallel default(none) shared(N, x, y, z, scale) private(f, i, j) num_threads(8) {
    f = 1.0;
    #pragma omp for nowait
    for (i = 0; i < N; i++) {
        z[i] = x[i] + y[i];
        complexcompute(omp_get_thread_num());
    }
    #pragma omp single
    complexcompute(200);
    #pragma omp single
    scale = sum(z, 0, N) + f;
}
```

> **`nowait` 使用条件**：需保证程序正确性。以下情况下可安全使用 `nowait`：
> 1. 使用默认的静态调度策略
> 2. 两个循环有相同的迭代次数
> 3. 循环绑定到同一个并行区
>
> **`nowait` 使用限制**：
> - 不能用于包含归约操作（`reduction`）的循环
> - 不能用于包含 `ordered` 子句的循环
> - 后续代码不能依赖于该循环的计算结果
> - 使用 `dynamic` 或 `guided` 调度时需谨慎，因为迭代完成顺序不确定

---

## 8. 流水并行优化

### 8.1 DOALL 与 DOACROSS 循环

根据循环蕴含的并行性不同，可分为：

- **DOALL 循环**：各次迭代之间不存在依赖关系，迭代间不需要同步，是完全并行。
- **DOACROSS 循环**：存在跨迭代的依赖关系，需要通过迭代间的同步实现并行执行。根据依赖距离可细分为规则 DOACROSS 和不规则 DOACROSS 循环，规则 DOACROSS 更容易通过流水并行方式进行优化。

### 8.2 流水并行步骤

以有限差分松弛法（FDR）循环为例，流水并行的步骤：

1. **判断**：目的循环是否为 DOACROSS 循环且循环层数大于 2
2. **选择**：计算划分层和循环分块层（如选择 j 层为计算划分层，i 层为循环分块层）
3. **同步**：使用计数信号量机制实现线程同步（左同步 + 右同步）
4. **优化**：考虑线程数有限的情况进行负载优化

### 8.3 同步实现

**左同步函数：**

```c
int isync[256], mthreadnum, iam;
#pragma omp threadprivate(mthreadnum, iam)

void sync_left() {
    int neighbour;
    if (iam > 0 && iam <= mthreadnum) {
        neighbour = iam - 1;
        while (isync[neighbour] == 0) {
            #pragma omp flush(isync)
        }
        isync[neighbour] = 0;
        #pragma omp flush(isync, a)
    }
}
```

**右同步函数：**

```c
void sync_right() {
    if (iam < mthreadnum) {
        while (isync[iam] == 1) {
            #pragma omp flush(isync)
        }
        #pragma omp flush(isync, a)
        isync[iam % (mthreadnum - 1)] = 1;
        #pragma omp flush(isync)
    }
}
```

### 8.4 流水并行粒度

流水计算粒度是指同一线程两次同步之间的计算工作量大小。通过**循环交换**和**循环分块**来调节流水粒度，平衡并行粒度和同步代价：

- **细粒度流水**：计算划分层放在循环嵌套较内层，同步代价较高
- **粗粒度流水**：计算划分层放在循环嵌套较外层，同步代价较小

#### 循环分块增大粒度

```c
int b = 9;  // 分块大小
for (int i = 1; i < N1 - 1; i = i + b) {
    sync_left();
    #pragma omp for schedule(static) nowait
    for (int j = 1; j < N2 - 1; j++) {
        for (int m = i; m < min(i + b, N1 - 1); m++) {
            a[m][j] = 0.25 * (a[m-1][j] + a[m][j-1] + a[m+1][j] + a[m][j+1]);
        }
    }
    sync_right();
}
```

#### 循环分段减小粒度

```c
int b = 4;  // 分块大小
for (int j = 1; j < N2 - 1; j++) {
    sync_left();
    for (int i = 1; i < N1 - 1; i += b) {
        for (int m = i; m < min(i + b, N1 - 1); m++) {
            a[m][j] = 0.25 * (a[m-1][j] + a[m][j-1] + a[m+1][j] + a[m][j+1]);
        }
    }
    sync_right();
}
```

#### 循环分段 + 循环交换减小粒度

```c
int b = 9;  // 分块大小
for (int j = 1; j < N2 - 1; j = j + b) {
    sync_left();
    #pragma omp for schedule(static) nowait
    for (int i = 1; i < N1 - 1; i++) {
        for (int m = j; m < min(j + b, N2 - 1); m++) {
            a[i][m] = 0.25 * (a[i-1][m] + a[i][m-1] + a[i+1][m] + a[i][m+1]);
        }
    }
    sync_right();
}
```

### 8.5 分块大小选择

通常需要建立代价模型来计算最优分块大小。代价因素包括：计算划分层和循环分块层的迭代数、线程数目、单个分块执行时间、同步开销时间、线程执行的分块数等。

**经验准则：** 当单个分块的同步开销时间与执行时间之比大于 1 时，采用增大粒度策略，分块大小设为两者比值取整；否则采用减小粒度策略。

> **结论**：流水并行优化的核心在于平衡**并行度**和**同步开销**。粒度过细则同步频繁，抵消并行收益；粒度过粗则线程间依赖链拉长，并行度降低。最优粒度取决于具体的计算模式和硬件平台，通常需要通过实验确定。

---

## 9. 访存优化

OpenMP 程序的性能不仅取决于计算并行度，还受到内存访问模式和数据局部性的显著影响。在多核多插槽系统上，NUMA 架构和缓存层次结构使得访存优化成为关键。

### 9.1 NUMA 感知内存分配

在 NUMA（Non-Uniform Memory Access）架构中，每个处理器拥有本地内存，访问本地内存的速度远快于访问远程内存。OpenMP 程序的内存分配策略直接影响访存延迟。

**First-Touch 策略**：Linux 默认采用 first-touch 策略——首次访问某内存页的线程决定该页的物理位置。因此，应在并行区内初始化数据，使数据分布到各线程对应的本地内存。

```c
// 错误做法：串行初始化导致所有数据分配到主线程所在节点
double A[N];
for (int i = 0; i < N; i++)
    A[i] = 0.0;

// 正确做法：并行初始化实现数据分布
double A[N];
#pragma omp parallel for
for (int i = 0; i < N; i++)
    A[i] = 0.0;
```

**NUMA 控制工具：**

```shell
# 查看系统 NUMA 拓扑
numactl --hardware

# 交错分配内存到所有 NUMA 节点（适用于全局共享数据）
numactl --interleave=all ./program

# 绑定进程到指定 NUMA 节点
numactl --cpunodebind=0 --membind=0 ./program
```

> **结论**：对于计算密集型程序，使用 first-touch 策略将数据分布到各节点可显著减少远程访存。对于全局共享的只读数据，使用 `--interleave=all` 可避免单节点成为瓶颈。

### 9.2 数据布局优化

缓存友好的数据布局是提升访存效率的关键。C 语言采用行优先存储，内层循环应沿行方向（最后的维度）遍历以利用空间局部性。

**优化前（缓存不友好）：**

```c
// 按列遍历——跨行访问导致缓存行利用率低
#pragma omp parallel for
for (int j = 0; j < N; j++)
    for (int i = 0; i < N; i++)
        A[i][j] = B[i][j] + C[i][j];
```

**优化后（缓存友好）：**

```c
// 按行遍历——连续访问充分利用缓存行
#pragma omp parallel for
for (int i = 0; i < N; i++)
    for (int j = 0; j < N; j++)
        A[i][j] = B[i][j] + C[i][j];
```

**循环分块（Loop Tiling）**：当数据规模超出缓存容量时，通过分块使工作集保持在缓存中。

```c
int BLOCK = 64;  // 分块大小根据 L1 缓存容量调整
#pragma omp parallel for collapse(2)
for (int ii = 0; ii < N; ii += BLOCK)
    for (int jj = 0; jj < N; jj += BLOCK)
        for (int i = ii; i < min(ii + BLOCK, N); i++)
            for (int j = jj; j < min(jj + BLOCK, N); j++)
                A[i][j] = B[i][j] + C[i][j];
```

> **结论**：数据布局优化的核心原则是**提升空间局部性**——让连续访问的数据在内存中也连续存放。循环交换和循环分块是两种常用手段，可与并行化结合使用。

### 9.3 线程亲和性设置

线程亲和性（Thread Affinity）控制线程与物理处理器核心的绑定关系，对缓存利用率和 NUMA 访存有重要影响。

**环境变量配置：**

```shell
# 设置线程绑定策略
export OMP_PROC_BIND=close      # 线程紧密绑定到相邻核心
export OMP_PROC_BIND=spread     # 线程均匀分散到各核心
export OMP_PROC_BIND=master     # 线程与主线程绑定到同一核心

# 指定线程可放置的处理器集合
export OMP_PLACES=cores         # 以物理核心为单位
export OMP_PLACES=threads       # 以硬件线程（超线程）为单位
export OMP_PLACES=sockets       # 以插槽为单位
```

**子句配置：**

```c
#pragma omp parallel proc_bind(close)
{
    // 线程紧密绑定，适合需要频繁访问共享数据的场景
}

#pragma omp parallel proc_bind(spread)
{
    // 线程分散绑定，适合各线程独立工作的场景
}
```

**绑定策略选择建议：**

| 策略      | 适用场景                                       |
| --------- | ---------------------------------------------- |
| `close`   | 线程间数据共享频繁，需要利用共享缓存           |
| `spread`  | 线程独立工作，需要最大化各线程的缓存和内存带宽 |
| `primary` | 线程需要访问主线程所在节点的数据               |

> **结论**：线程亲和性的设置应根据程序的数据访问模式选择。`close` 策略适合共享数据较多的场景（利用 L3 缓存），`spread` 策略适合独立计算较多的场景（利用各节点的本地内存带宽）。

---

## 总结

| 优化技术     | 核心思想                                    | 性能收益来源         |
| ------------ | ------------------------------------------- | -------------------- |
| 并行区重构   | 通过并行区扩张和合并，减少线程创建/合并开销 | 减少 fork-join 开销  |
| 避免伪共享   | 通过数据填充或数据私有化，消除缓存行冲突    | 减少缓存一致性流量   |
| 循环向量化   | 结合多线程并行与 SIMD 向量执行              | 提升单核计算吞吐量   |
| 负载均衡优化 | 利用 collapse、调度策略使各线程工作量均衡   | 减少线程空闲等待     |
| 线程数设置   | 根据程序特征和运行环境选择最优线程数        | 平衡并行收益与开销   |
| 避免隐式同步 | 使用 `nowait` 消除不必要的同步点            | 减少同步等待时间     |
| 流水并行     | 通过同步机制发掘 DOACROSS 循环的并行性      | 挖掘跨迭代并行性     |
| 访存优化     | NUMA 感知分配、数据布局优化、线程亲和性     | 减少远程内存访问延迟 |
