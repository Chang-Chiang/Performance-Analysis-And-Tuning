# Loop Interchange (Gaussian Blur)

> **环境准备**
>
> ```bash
> # 将 CPU 调频策略设为 performance，锁定最高频率，避免动态调频干扰 benchmark 稳定性
> sudo cpupower frequency-set --governor performance
>
> # 创建 build 目录并进入（out-of-source build，保持源码目录干净）
> cmake -E make_directory build && cd build
>
> # 配置构建：Release 模式开启优化（-O3），同时加 -g 保留调试符号以便 perf 定位源码行
> cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_FLAGS="-g" -DCMAKE_CXX_FLAGS="-g" ..
>
> # 编译，8 线程并行加速构建
> cmake --build . --config Release --parallel 8
> ```

## 背景

高斯模糊（1×5 可分离核 `{1,4,6,4,1}`）对 6856×4306 灰度图做垂直方向滤波。核心是 `filterVertically()` 中间部分——对每列独立累加。

关键数据：图像 width=6856, height=4306, 每像素 1B, 总计 ~29.5MB。

---

## 优化前

`filterVertically()` 的中间部分采用 c-outer、r-inner 的循环顺序：

```c++
for (int c = 0; c < width; c++) {           // 外层遍历列
    for (int r = radius; r < height - radius; r++) {  // 内层遍历行
        int dot = 0;
        for (int i = 0; i < radius + 1 + radius; i++) {
            dot += input[(r - radius + i) * width + c] * kernel[i];
            //                ↑ 每次跳一整行 (stride = 6856B)
        }
        int value = (dot + rounding) >> shift;
        output[r * width + c] = static_cast<uint8_t>(value);
    }
}
```

**问题：** 内层 `r` 循环访问 `input[(r-radius+i)*width+c]`，相邻迭代跳 6856B（一整行），每次访问都 miss cache line。图像 29.5MB 远超 L3（36MB），预取无法弥补。

```shell
$ cmake --build build --target benchmarkLab && cd build && ./lab ../pexels-pixabay-434334.pbm output.pgm --benchmark_min_time=2s
-----------------------------------------------------
Benchmark           Time             CPU   Iterations
-----------------------------------------------------
bench1      123081113 ns    123061415 ns           23
```

```shell
$ toplev.py --core S0-C0 -l2 --no-desc taskset -c 0 ./lab ../pexels-pixabay-434334.pbm output.pgm --benchmark_min_time=0.5s
core BE               Backend_Bound               % Slots                       89.0
core BE/Mem           Backend_Bound.Memory_Bound   % Slots                       56.9  <==
core BE/Core          Backend_Bound.Core_Bound     % Slots                       32.2
```

**瓶颈：** Memory Bound 56.9%，CPU 大量时间在等待内存数据。Backend Bound 高达 89.0%，流水线严重堵塞。

---

## 优化后

经过 Loop Interchange + 栈分配 + 内层循环重排（详见[附录](#附录)完整步骤），最终代码：

```c++
for (int r = radius; r < height - radius; r++) {  // 外层遍历行
    int dot[width];  // 栈分配，~27KB，L1 热数据
    for (int c = 0; c < width; c++) {
        dot[c] = 0;
    }

    for (int i = 0; i < radius + 1 + radius; i++) {  // kernel 常驻寄存器
        for (int c = 0; c < width; c++) {             // 内层连续扫描行
            dot[c] += input[(r - radius + i) * width + c] * kernel[i];
            //                          ↑ stride = 1B，cache line 友好
        }
    }

    for (int c = 0; c < width; c++) {
        int value = (dot[c] + rounding) >> shift;
        output[r * width + c] = static_cast<uint8_t>(value);
    }
}
```

```shell
$ cmake --build build --target benchmarkLab && cd build && ./lab ../pexels-pixabay-434334.pbm output.pgm --benchmark_min_time=2s
-----------------------------------------------------
Benchmark           Time             CPU   Iterations
-----------------------------------------------------
bench1       11785819 ns     11784632 ns          233
```

```shell
$ toplev.py --core S0-C0 -l2 --no-desc taskset -c 0 ./lab ../pexels-pixabay-434334.pbm output.pgm --benchmark_min_time=0.5s
core BE               Backend_Bound               % Slots                       58.4
core BE/Mem           Backend_Bound.Memory_Bound   % Slots                       24.3
core BE/Core          Backend_Bound.Core_Bound     % Slots                       34.1  <==
```

---

## 优化分析

### 性能对比

| 指标           | 优化前        | 优化后       | 变化              |
| -------------- | ------------- | ------------ | ----------------- |
| 耗时           | 123.1 ms      | 11.8 ms      | **10.4x 加速**    |
| Memory Bound   | 56.9%         | 24.3%        | **-32.6%**        |
| Core Bound     | 32.2%         | 34.1%        | 持平（成为新瓶颈）|
| Backend Bound  | 89.0%         | 58.4%        | **-30.6%**        |

### 为什么有效

**1. Loop Interchange — 消除跨行访问**

原始 c-outer/r-inner 循环中，`input[(r-radius+i)*width+c]` 相邻迭代 stride=6856B，每个 cache line（64B）只用到 1 个字节，利用率 1/64。交换为 r-outer/c-inner 后，内层连续扫描一行（stride=1B），cache line 利用率接近 100%。

**2. 栈分配 `dot[width]` — 提升 L1 命中率**

`dot[width]`（6856×4B ≈ 27KB）分配在栈上，与当前执行上下文空间局部性强。相比堆分配的 `new int[height*width]`（~112MB），栈数组不会挤占 L3 cache，且编译器可将其完全保留在寄存器/L1 中。

**3. i-c 内层交换 — kernel 常驻寄存器**

将 `i`（kernel 索引）提到外层，`c`（列索引）放在内层。5-tap kernel 只需从 `kernel[i]` 加载 5 次，编译器将其分配到寄存器；内层 `c` 循环连续访问 `dot[c]` 和 `input[...+c]`，自动向量化（auto-vectorization）生效。

### 内存访问模式

```
input 图像（行优先存储）：
  row r-2: [████████████████████████████]  ← 连续 6856B
  row r-1: [████████████████████████████]
  row r  : [████████████████████████████]
  row r+1: [████████████████████████████]
  row r+2: [████████████████████████████]

优化前 (c-outer): 跳行访问 input[(r-2)*width+c] → input[(r-1)*width+c] → ...
                  stride = 6856B，每条 cache line 只用 1B ✗

优化后 (r-outer): 固定 r，遍历 c=0..width-1
                  stride = 1B，每条 cache line 用满 64B ✓
```

### 验证正确性

```shell
$ cd build && ./validate ../pexels-pixabay-434334.pbm ../output-golden.pgm
Validation Successful
```

## 附录

以下是逐步优化的完整过程，每一步都在前一步基础上做单一变换，展示从原始代码到最终优化版本的演进路径。

---

### Step 1. 原始代码

**说明：** 原始实现采用 c-outer、r-inner 的循环顺序。由于 `input` 按行优先存储（`input[row * width + col]`），内层循环遍历 `r` 时每次跳转一整行（stride = 6856B），导致严重的 cache miss。三个区域（top/middle/bottom）嵌套在同一个 `c` 循环内。

```c++
static void filterVertically(
    uint8_t* output, const uint8_t* input, const int width, const int height, const int* kernel,
    const int radius, const int shift) {
    const int rounding = 1 << (shift - 1);

    for (int c = 0; c < width; c++) {

        // Top part of line, partial kernel
        for (int r = 0; r < std::min(radius, height); r++) {
            // Accumulation
            int  dot = 0;
            int  sum = 0;
            auto p   = &kernel[radius - r];
            for (int y = 0; y <= std::min(r + radius, height - 1); y++) {
                int weight = *p++;
                dot += input[y * width + c] * weight;
                sum += weight;
            }

            // Normalization
            int value             = static_cast<int>(dot / static_cast<float>(sum) + 0.5f);
            output[r * width + c] = static_cast<uint8_t>(value);
        }


        // Middle part of computations with full kernel
        for (int r = radius; r < height - radius; r++) {
            // Accumulation
            int dot = 0;
            for (int i = 0; i < radius + 1 + radius; i++) {
                dot += input[(r - radius + i) * width + c] * kernel[i];
            }

            // Fast shift instead of division
            int value             = (dot + rounding) >> shift;
            output[r * width + c] = static_cast<uint8_t>(value);
        }


        // Bottom part of line, partial kernel
        for (int r = std::max(radius, height - radius); r < height; r++) {
            // Accumulation
            int  dot = 0;
            int  sum = 0;
            auto p   = kernel;
            for (int y = r - radius; y < height; y++) {
                int weight = *p++;
                dot += input[y * width + c] * weight;
                sum += weight;
            }

            // Normalization
            int value             = static_cast<int>(dot / static_cast<float>(sum) + 0.5f);
            output[r * width + c] = static_cast<uint8_t>(value);
        }
    }
}

```

---

### Step 2. Loop Distribute（拆分三个区域为独立循环）

**说明：** 将 Step 1 中嵌套在 `c` 循环内的 top/middle/bottom 三个区域拆分为各自独立的 `c` 循环。这是后续变换的前置步骤——只有将中间部分（middle）独立出来，才能单独对它进行数组化和循环交换。功能等价，不做性能优化。

```c++
// Applies Gaussian blur in independent vertical lines
static void filterVertically(
    uint8_t* output, const uint8_t* input, const int width, const int height, const int* kernel,
    const int radius, const int shift) {
    const int rounding = 1 << (shift - 1);

    // Top part of line, partial kernel
    for (int c = 0; c < width; c++) {
        for (int r = 0; r < std::min(radius, height); r++) {
            // Accumulation
            int  dot = 0;
            int  sum = 0;
            auto p   = &kernel[radius - r];
            for (int y = 0; y <= std::min(r + radius, height - 1); y++) {
                int weight = *p++;
                dot += input[y * width + c] * weight;
                sum += weight;
            }

            // Normalization
            int value             = static_cast<int>(dot / static_cast<float>(sum) + 0.5f);
            output[r * width + c] = static_cast<uint8_t>(value);
        }
    }

    // Middle part of computations with full kernel
    for (int c = 0; c < width; c++) {
        for (int r = radius; r < height - radius; r++) {
            // Accumulation
            int dot = 0;
            for (int i = 0; i < radius + 1 + radius; i++) {
                dot += input[(r - radius + i) * width + c] * kernel[i];
            }

            // Fast shift instead of division
            int value             = (dot + rounding) >> shift;
            output[r * width + c] = static_cast<uint8_t>(value);
        }
    }

    // Bottom part of line, partial kernel
    for (int c = 0; c < width; c++) {
        for (int r = std::max(radius, height - radius); r < height; r++) {
            // Accumulation
            int  dot = 0;
            int  sum = 0;
            auto p   = kernel;
            for (int y = r - radius; y < height; y++) {
                int weight = *p++;
                dot += input[y * width + c] * weight;
                sum += weight;
            }

            // Normalization
            int value             = static_cast<int>(dot / static_cast<float>(sum) + 0.5f);
            output[r * width + c] = static_cast<uint8_t>(value);
        }
    }
}
```

---

### Step 3. 提出待优化代码段

**说明：** 只保留 middle 部分（使用完整 kernel，无需归一化），忽略 top/bottom 边界处理。这是性能瓶颈所在——占总计算量的绝大部分，且 kernel 大小固定（radius=2, 5-tap），适合做进一步变换。后续所有步骤只针对此代码段。

```c++
// Middle part of computations with full kernel
for (int c = 0; c < width; c++) {
    for (int r = radius; r < height - radius; r++) {
        // Accumulation
        int dot = 0;
        for (int i = 0; i < radius + 1 + radius; i++) {
            dot += input[(r - radius + i) * width + c] * kernel[i];
        }

        // Fast shift instead of division
        int value             = (dot + rounding) >> shift;
        output[r * width + c] = static_cast<uint8_t>(value);
    }
}
```

---

### Step 4. 中间变量修改为数组

**说明：** 将标量 `dot` 改为数组 `dot[height - radius]`，每个 `r` 对应一个独立的累加结果。这打破了 `dot` 的循环依赖（每轮迭代各自独立），为下一步将累加与输出拆分为两个循环做准备。此时数组按 `r` 索引，仍在 c-outer 循环内。

```c++
for (int c = 0; c < width; c++) {
    int dot[height - radius];
    for (int r = radius; r < height - radius; r++) {
        dot[r] = 0;
        for (int i = 0; i < radius + 1 + radius; i++) {
            dot[r] += input[(r - radius + i) * width + c] * kernel[i];
        }

        int value             = (dot[r] + rounding) >> shift;
        output[r * width + c] = static_cast<uint8_t>(value);
    }
}

```

---

### Step 5. Loop Distribute（拆分累加与输出）

**说明：** 将 Step 4 中的单个 r 循环拆分为两个：第一个只做累加（计算 `dot[r]`），第二个做归一化并写入 `output`。拆分后两个循环遍历相同的 `r` 范围，但职责单一。这是为后续扩大数组维度、交换循环顺序做铺垫。

```c++
for (int c = 0; c < width; c++) {
    int dot[height - radius];
    for (int r = radius; r < height - radius; r++) {
        dot[r] = 0;
        for (int i = 0; i < radius + 1 + radius; i++) {
            dot[r] += input[(r - radius + i) * width + c] * kernel[i];
        }
    }

    for (int r = radius; r < height - radius; r++) {
        int value             = (dot[r] + rounding) >> shift;
        output[r * width + c] = static_cast<uint8_t>(value);
    }
}
```

---

### Step 6. 扩展为更宽的数组（2D 布局）

**说明：** 将一维数组 `dot[r]`（按列分配）扩展为二维布局 `dot[r * width + c]`（堆上分配 `new int[(height - radius) * width]`）。数据排列从按列存储变为按行存储，与 `input` 和 `output` 的布局一致。这使得后续将循环从 c-outer 交换为 r-outer 时，`dot` 的访问也能变为连续的。

```c++
int *dot = new int[(height - radius) * width];

for (int c = 0; c < width; c++) {

    for (int r = radius; r < height - radius; r++) {

        dot[r * width + c] = 0;

        for (int i = 0; i < radius + 1 + radius; i++) {
            dot[r * width + c] += input[(r - radius + i) * width + c] * kernel[i];
        }
    }

    for (int r = radius; r < height - radius; r++) {
        int value             = (dot[r * width + c] + rounding) >> shift;
        output[r * width + c] = static_cast<uint8_t>(value);
    }
}
```

---

### Step 7. Loop Distribute（拆分 c 循环为累加与输出）

**说明：** 将 Step 6 中 c-outer 循环内的两个 r 循环（累加 + 输出）拆分为两个独立的 c-outer 循环。第一个 c 循环完成所有 `dot` 的累加，第二个 c 循环完成所有 `output` 的写入。两者完全解耦后，才能对累加循环做 r/c 交换。

```c++
int *dot = new int[(height - radius) * width];

for (int c = 0; c < width; c++) {
    for (int r = radius; r < height - radius; r++) {
        dot[r * width + c] = 0;
        for (int i = 0; i < radius + 1 + radius; i++) {
            dot[r * width + c] += input[(r - radius + i) * width + c] * kernel[i];
        }
    }
}

for (int c = 0; c < width; c++) {
    for (int r = radius; r < height - radius; r++) {
        int value             = (dot[r * width + c] + rounding) >> shift;
        output[r * width + c] = static_cast<uint8_t>(value);
    }
}
```

---

### Step 8. Loop Interchange（r/c 交换）

**说明：** **核心优化步骤。** 将累加循环和输出循环的外层从 `c` 交换为 `r`。交换后，内层遍历 `c` 时 `input[(r - radius + i) * width + c]` 变为行优先连续访问（stride=1B），cache line 利用率从 ~1/6856 提升到接近 100%。这是从 123ms 降到 58ms 的关键步骤。

```c++
int *dot = new int[(height - radius) * width];

for (int r = radius; r < height - radius; r++) {
    for (int c = 0; c < width; c++) {
        dot[r * width + c] = 0;
        for (int i = 0; i < radius + 1 + radius; i++) {
            dot[r * width + c] += input[(r - radius + i) * width + c] * kernel[i];
        }
    }
}

for (int r = radius; r < height - radius; r++) {
    for (int c = 0; c < width; c++) {
        int value             = (dot[r * width + c] + rounding) >> shift;
        output[r * width + c] = static_cast<uint8_t>(value);
    }
}
```

---

### Step 9. Loop Distribute（拆分初始化与累加）

**说明：** 将 r-outer 循环内的 `dot[r * width + c] = 0` 初始化和累加计算拆分为两个独立的 c-inner 循环。目的是让零初始化成为一个独立的、简单的 memset 模式，编译器可以将其识别并优化为 `memset` 或向量化清零指令。

```c++
int *dot = new int[(height - radius) * width];

for (int r = radius; r < height - radius; r++) {

    for (int c = 0; c < width; c++) {
        dot[r * width + c] = 0;
    }

    for (int c = 0; c < width; c++) {
        for (int i = 0; i < radius + 1 + radius; i++) {
            dot[r * width + c] += input[(r - radius + i) * width + c] * kernel[i];
        }
    }
}

for (int r = radius; r < height - radius; r++) {
    for (int c = 0; c < width; c++) {
        int value             = (dot[r * width + c] + rounding) >> shift;
        output[r * width + c] = static_cast<uint8_t>(value);
    }
}
```

---

### Step 10. Loop Distribute（将初始化提升为独立外层循环）

**说明：** 将零初始化从 r-outer 循环中完全分离，成为独立的双层循环。这样三个阶段（初始化、累加、输出）各自独立遍历整个 `dot` 数组。分离后，初始化循环可以被编译器优化为批量 memset，且不再与累加循环争夺 cache。

```c++
int *dot = new int[(height - radius) * width];

for (int r = radius; r < height - radius; r++) {
    for (int c = 0; c < width; c++) {
        dot[r * width + c] = 0;
    }
}

for (int r = radius; r < height - radius; r++) {
    for (int c = 0; c < width; c++) {
        for (int i = 0; i < radius + 1 + radius; i++) {
            dot[r * width + c] += input[(r - radius + i) * width + c] * kernel[i];
        }
    }
}

for (int r = radius; r < height - radius; r++) {
    for (int c = 0; c < width; c++) {
        int value             = (dot[r * width + c] + rounding) >> shift;
        output[r * width + c] = static_cast<uint8_t>(value);
    }
}
```

---

### Step 11. Loop Interchange（i/c 交换）

**说明：** 在累加阶段，将内层两层循环从 `c-inner, i-inner` 交换为 `i-inner, c-inner`。交换后，`kernel[i]` 在外层 `i` 循环中只需加载 5 次（radius=2, 5-tap kernel），可常驻寄存器；内层 `c` 循环连续访问 `dot[r * width + c]` 和 `input[...+c]`，都是 stride=1B 的行扫描。进一步减少内存访问开销。

```c++
int *dot = new int[(height - radius) * width];

for (int r = radius; r < height - radius; r++) {
    for (int c = 0; c < width; c++) {
        dot[r * width + c] = 0;
    }
}

for (int r = radius; r < height - radius; r++) {
    for (int i = 0; i < radius + 1 + radius; i++) {
        for (int c = 0; c < width; c++) {
            dot[r * width + c] += input[(r - radius + i) * width + c] * kernel[i];
        }
    }
}

for (int r = radius; r < height - radius; r++) {
    for (int c = 0; c < width; c++) {
        int value             = (dot[r * width + c] + rounding) >> shift;
        output[r * width + c] = static_cast<uint8_t>(value);
    }
}
```

---

### Step 12. Loop Merge（合并三个 r 循环）

**说明：** 将初始化、累加、输出三个独立的 r-outer 循环合并回一个 r-outer 循环。因为三者遍历相同的 `r` 范围（`radius` 到 `height - radius`），合并后 `dot` 数组的生命周期缩短到单次 r 迭代，cache 局部性更好——每一行的 `dot[]` 在 L1 中完成初始化、累加、输出后即可丢弃，不会污染后续行的数据。

```c++
int *dot = new int[(height - radius) * width];

for (int r = radius; r < height - radius; r++) {
    for (int c = 0; c < width; c++) {
        dot[r * width + c] = 0;
    }

    for (int i = 0; i < radius + 1 + radius; i++) {
        for (int c = 0; c < width; c++) {
            dot[r * width + c] += input[(r - radius + i) * width + c] * kernel[i];
        }
    }

    for (int c = 0; c < width; c++) {
        int value             = (dot[r * width + c] + rounding) >> shift;
        output[r * width + c] = static_cast<uint8_t>(value);
    }
}
```

---

### Step 13. 数组移入循环内（栈分配）

**说明：** **最终优化版本。** 将堆分配的 `new int[(height - radius) * width]`（~112MB）改为栈上局部数组 `int dot[width]`（~27KB）。栈内存紧邻当前执行上下文，L1 cache 命中率更高；消除了堆分配的开销和堆内存对 cache 的污染；`dot` 从二维索引 `r * width + c` 简化为一维索引 `c`，因为每次 r 迭代只处理一行。从 58ms 进一步降到 12.7ms。

```c++
for (int r = radius; r < height - radius; r++) {
    int dot[width];
    for (int c = 0; c < width; c++) {
        dot[c] = 0;
    }

    for (int c = 0; c < width; c++) {
        for (int i = 0; i < radius + 1 + radius; i++) {
            dot[c] += input[(r - radius + i) * width + c] * kernel[i];
        }
    }

    for (int c = 0; c < width; c++) {
        int value             = (dot[c] + rounding) >> shift;
        output[r * width + c] = static_cast<uint8_t>(value);
    }
}
```