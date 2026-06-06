### 5. 访存优化

现代计算机采用多级层次存储结构平衡成本、容量与速度：

```
寄存器（Register）        ← 最快、最小、最贵
  ↓
高速缓存（Cache）          ← SRAM，集成在处理器内部
  ↓
主存储器（Main Memory）    ← DRAM
  ↓
辅助存储器（Disk）         ← 硬盘、光盘等
  ↑ 容量增大，速度和成本降低
```

#### 寄存器优化

**减少全局变量**：全局变量会独占寄存器，减少过程内可分配寄存器数量。应尽量将全局变量改为局部变量。

```c
// 优化前：全局变量 x 独占寄存器
float x = 5.5642;
int function(float *a, int N) {
    for (int i = 0; i < N; i++)
        a[i] = x * 2.541;
}

// 优化后：局部变量，寄存器可更灵活分配
int function(float *a, int N) {
    float x = 5.5642;
    for (int i = 0; i < N; i++)
        a[i] = x * 2.541;
}
```

**标量替换**：将数组访问替换为标量变量，直接读取寄存器中的数据，避免每次都从缓存加载。

```c
// 优化前：a[i] 和 b[i] 每次都从缓存读取
for (int i = 0; i < n; i++) {
    c[i] = a[i] + b[i];
    d[i] = a[i] - b[i];
}

// 优化后：用标量变量缓存，减少内存读取
for (int i = 0; i < n; i++) {
    float x = a[i];  // 加载到寄存器
    float y = b[i];
    c[i] = x + y;
    d[i] = x - y;
}
```

**减少内存写**：将累加操作保存在寄存器中，减少对数组的反复写入。

```c
// 优化前：每次迭代都写 a[i]
for (int i = 0; i < n; i++)
    for (int j = 0; j < n; j++)
        a[i] = a[i] + b[i][j];

// 优化后：用 sum 在寄存器中累加，最后写一次
for (int i = 0; i < n; i++) {
    int sum = a[i];
    for (int j = 0; j < n; j++)
        sum = sum + b[i][j];
    a[i] = sum;
}
```

**防止寄存器溢出**：当所需寄存器数量大于可分配数量时，编译器会将变量溢出到栈上（额外的 STORE/LOAD），抵消优化效果。

```asm
STORE R1, 0(SP)      ; 寄存器溢出：保存到栈
LOOP:
  LOAD R1, b[i]      ; 正常加载
  LOAD R2, c[i]
  MUL R1, R2, R1
  STORE R1, a[i]
LOOP END
LOAD R1, 0(SP)       ; 寄存器溢出：从栈恢复
```

**寄存器重用**：数据加载到寄存器后，尽量保留在寄存器中，避免重复从缓存读取。

```c
// 优化前：A[i] 在每次 j 迭代中重复加载
for (int i = ii; i < min(ii + Ti, NI); ++i)
    for (int j = 0; j < NJ; ++j)
        A[i] += B[j * NI + i];

// 循环展开 + 寄存器重用：A[i] 只加载一次，复用 4 次
for (int i = ii; i < min(ii + Ti, NI); ++i) {
    for (int j = 0; j < NJ; j += 4) {
        A[i] += B[j * NI + i];
        A[i] += B[(j + 1) * NI + i];
        A[i] += B[(j + 2) * NI + i];
        A[i] += B[(j + 3) * NI + i];
    }
}
```

#### 缓存优化

处理器缓存分为一级缓存（指令缓存 + 数据缓存）、二级缓存和三级缓存，缓和了处理器与主存之间速度不匹配的矛盾。

**缓存分块**：将大矩阵分割为小矩阵，使每次计算的数据能够放入缓存，提高缓存命中率。

```c
// 优化前：i=1 时需访问 z 的全部 N*N 个元素，缓存命中率低
for (int i = 0; i < N; i++)
    for (int j = 0; j < N; j++) {
        int r = 0;
        for (int k = 0; k < N; k++)
            r += y[i][k] * z[k][j];
        x[i][j] = r;
    }

// 缓存分块：S 为分块大小，每次只访问小矩阵
for (int jj = 0; jj < N; jj += S)
    for (int kk = 0; kk < N; kk += S)
        for (int i = 0; i < N; i++)
            for (int j = jj; j < min(jj + S, N); j++) {
                int r = 0;
                for (int k = kk; k < min(kk + S, N); k++)
                    r += y[i][k] * z[k][j];
                x[i][j] += r;
            }
```

测试结果：固定分块大小 50，矩阵规模增大时分块优势越来越明显；固定矩阵规模 1024，分块大小 128 时效果最佳。

**数组分类与缓存映射**：根据矩阵行长度与缓存行大小的关系，数组可分为三类：

| 类型 | 特征 | 缓存命中率 |
| ---- | ---- | ---------- |
| 一类 | 行长度是缓存行大小的整数倍 | 最高 |
| 二类 | 行长度不是缓存行大小的整数倍 | 较低（可能跨缓存行） |
| 三类 | 行长度是整个缓存大小的整数倍 | 最低（列数据映射到同一缓存组） |

通过**行列扩充**将二类数组变为一类数组可提升性能。例如将 251×251 矩阵扩充为 256×256（补零），加速比约 1.6 倍。

**减少伪共享**：多核系统中，不同变量处于同一缓存行时，某核心修改数据会使其他核心的缓存行失效，导致大量缓存冲突。

```c
// 伪共享：sum 数组在同一缓存行，多线程同时写导致冲突
int sum[THREAD_NUM];
#pragma omp parallel for
for (int i = 0; i < THREAD_NUM; i++)
    for (int j = 0; j < N; j++)
        sum[i] += values[j] >> i;

// 优化后：使用局部变量减少共享缓存行写入
#pragma omp parallel for
for (int i = 0; i < THREAD_NUM; i++) {
    int local_sum = 0;
    for (int j = 0; j < N; j++)
        local_sum += values[j] >> i;
    sum[i] = local_sum;
}
```

可使用 `perf c2c` 工具分析伪共享问题。

**数据预取**：将待处理数据提前加载到缓存，避免缓存不命中。

| 预取类型 | 说明 |
| -------- | ---- |
| 无预取   | 等待缓存不命中后才加载 |
| 理想预取 | 数据在使用前恰好到达 |
| 非理想预取 | 数据过早或过晚到达 |

软件预取使用 `__builtin_prefetch(addr, rw, locality)` 函数，测试显示可提升约 13% 性能。

```c
__builtin_prefetch(mul1, 0, 3);  // 预取读，高局部性
__builtin_prefetch(mul2, 0, 0);  // 预取读，无局部性
__builtin_prefetch(res, 1, 3);   // 预取写，高局部性
for (int i = 0; i < N; i++)
    for (int j = 0; j < N; j++)
        for (int k = 0; k < N; k++)
            res[i][j] += mul1[i][k] * mul2[k][j];
```

#### 内存优化

**减少内存读写**：优先使用寄存器，通过保存临时中间结果减少内存访问。

```c
// 优化前：每次迭代都读写 a[i]
for (int i = 1; i < n; i++)
    a[i] += a[i - 1];

// 优化后：用 temp 保存中间结果
int temp = a[0];
for (int i = 1; i < n; i++) {
    temp += a[i];
    a[i] = temp;
}
```

**数据对齐**：处理器访问正确对齐的数据效率最高。未对齐数据需要多次内存访问。

**结构体对齐**：变量起始地址需被其对齐值整除。定义结构体时应按成员大小排列，大数据类型在前：

```c
// 优化前：12 字节（char 后需填充 3 字节对齐 int）
struct A1 { char a; int b; char c; short d; };

// 优化后：8 字节（紧凑排列）
struct A2 { char a; char c; short d; int b; };
```

**直接内存访问（DMA）**：DMA 可直接传输外围设备与主内存之间的数据，比缓存机制更高效——DMA 提前搬移数据，缓存是需要时才搬移。

**访存与计算重叠**：在指令层次将访存与计算重叠，隐藏访存延迟：

```asm
; 优化前：访存和计算串行
SUB R6, R7, R5
MUL R6, R7, R8
LOAD R1, a[i]
LOAD R2, b[i]

; 优化后：访存与计算并行
SUB R6, R7, R5
|| LOAD R1, a[i]    ; 与 SUB 并行
MUL R6, R7, R8
|| LOAD R2, b[i]    ; 与 MUL 并行
```

#### 磁盘优化

**多线程操作**：多线程随机读可达单线程的 10 倍以上，但会增大响应时间。

| 读线程数 | 100 次读耗时 | 平均响应时间 |
| -------- | ------------ | ------------ |
| 1        | 1329575      | 13294        |
| 10       | 149201       | 15989        |
| 50       | 96596        | 48355        |

**避免随机写**：顺序访问磁头几乎不用换道，随机写导致磁头频繁换道。可通过内存缓存排序，使磁头只向一个方向移动。

**磁盘预读**：Linux 采用窗口扩张预读策略，首次预读大小为读大小的 2 倍，逐次倍增至最大预读大小。

#### 数据布局

**数据重组**：将多个独立数组合并为结构体数组，提高访存局部性。

```c
// 优化前：a、b、c 分开存储，局部性差
for (int i = 0; i < n; i++)
    sum[i] = a[i] + b[i] + c[i];

// 优化后：合并为结构体数组
typedef struct { int a, b, c, sum; } arr_struct;
for (int i = 0; i < n; i++)
    arr[i].sum = arr[i].a + arr[i].b + arr[i].c;
```

**数据转置**：当最内层循环的数组索引方式与内存存放方式不同时，转置数组使访问连续。

```c
// 优化前：列访问，不连续
for (int i = 0; i < n; i++) {
    x[i][1] = x[i][1] + phi * y[i][1];
    x[i][2] = x[i][2] + phi * y[i][2];
}

// 优化后：行访问，连续
for (int i = 0; i < n; i++) {
    x[1][i] = x[1][i] + phi * y[1][i];
    x[2][i] = x[2][i] + phi * y[2][i];
}
```

**结构体属性域调整**：将常访问的属性域组织在一起，提高空间局部性。

```c
// 优化前：t_x、v_x、d_x 不连续
typedef struct { float t_x, t_y, t_z, v_x, v_y, v_z, d_x, d_y, d_z; } motion;

// 优化后：按维度组织，同维度数据连续
typedef struct { float t_x, v_x, d_x, t_y, v_y, d_y, t_z, v_z, d_z; } motion_1;
```

测试结果：100000 次迭代，调整前 494μs，调整后 395μs。

**结构体拆分**：将结构体按维度拆分，使相邻迭代的数据连续。

```c
typedef struct { float t_x, v_x, d_x; } motion_x;
typedef struct { float t_y, v_y, d_y, t_z, v_z, d_z; } motion_yz;
```

拆分后可进一步将**结构体数组转为数组结构体**，使同属性域相邻迭代数据连续：

```c
typedef struct {
    float Pt_x[N];  // t_x 相邻迭代连续
    float Pv_x[N];  // v_x 相邻迭代连续
    float Pd_x[N];  // d_x 相邻迭代连续
} motion_x;
```

访存优化从多层存储结构出发，按离处理器从近至远的顺序介绍了寄存器、缓存、内存、磁盘的优化方法，以及改善数据局部性的技巧。


