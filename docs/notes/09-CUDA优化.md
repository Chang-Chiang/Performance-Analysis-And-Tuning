# 09 · CUDA 程序优化

> **立意**：从"多厨师"跳到"**上千学徒流水线**"（GPU）。CPU 求**低延迟**、GPU 求**高吞吐 + 掩盖延迟**——所有手法归结两件事：**喂饱带宽 + 藏住延迟**。GPU 概念与前 7 课 CPU 概念**底层同构**（见 §六）。

---

## 一、Host/Device + 三级线程层次

**异构流程**（6 步固定套路）：
```
cudaMalloc         在 GPU 开显存
cudaMemcpy H2D     内存→显存  ⚠️ 数据搬运是最容易被忽略的巨大开销
kernel<<<grid,block>>>(...)  启动核函数
cudaMemcpy D2H     显存→内存
cudaFree / DeviceReset
```

**黄金法则**：**别让数据来回搬家**——GPU 算得再快，若数据搬运 > 计算时间就得不偿失。

**三级线程**：
```
Grid ─ 一次 kernel 启动
 └─ Block (如 32×32) ─ 映射到一个 SM (流式多处理器)
      └─ Thread ─ 算一个元素
row = blockIdx.y*blockDim.y + threadIdx.y   ← 二维坐标最直观
```

**入门代码**：[`basis/CUDA/Basics/cuda_programming_basics.cu`](../../basis/CUDA/Basics/cuda_programming_basics.cu)、[`matrix_multiply_basic.cu`](../../basis/CUDA/Basics/matrix_multiply_basic.cu)

---

## 二、线程结构优化：让所有 SM 都别闲着

**只用一个 block → 只有一个 SM 干活**，其他闲置（对应第 3 课硬件"多核没用满"）。开多 block 铺满所有 SM。

**⭐ 关键实测经验（工程文档基准）**：1024×1024 矩阵乘

| block 布局 (grid, block) | 时间 |
|------------------------|------|
| **((16,64),(64,16))** 或 ((32,32),(32,32)) | **~1.02s ✅ 最优** |
| ((1,1024),(1024,1)) 一维极端 | 8.5s ❌ 最差 |

> 👉 **铁律**：**用合理的二维布局（16×64 / 32×32），千万别用 (1,1024)**。warp 是 32 线程一束（下节），32×32 正好整齐切分、访存更易合并。
> 代码：[`thread_layout.cu`](../../basis/CUDA/ThreadStructure/thread_layout.cu)

---

## 三、Warp 与"线程束分化"（GPU 独有，头号隐形杀手）

- 硬件调度最小单位 = **warp（32 线程一束）**，它们以 **SIMT** 方式**同一条指令一起走**（一个学徒队听同一号令做同一动作）。
- GPU **没有复杂分支预测器**（第 6 课 CPU 有）。队里一旦遇到 `if`，走 A 分支和走 B 分支的线程**只能轮流串行执行**，另一拨干等 → **warp divergence（线程束分化）**，并行度暴跌。

**经典案例：并行归约的两种配对**（[`reduce_divergence.cu`](../../basis/CUDA/BranchOptimization/reduce_divergence.cu)）：
```c
// ✗ 相邻配对: if (tid % (2*stride)==0)  → 同 warp 内一半干一半等, 分化严重
// ✓ 交错配对: 让相邻线程做同样判断         → 分化消除
```
工程文档基准（16M 数据，RTX 3090）：**相邻 393μs → 交错 227μs，快约 1.7x**；内存吞吐 583→1050 GB/s。

> 呼应第 5 课"分支优化治 Bad Speculation"——GPU 上后果**放大 32 倍**（整束受害）。

---

## 四、⭐ 访存：GPU 的生死线

GPU 存储层次（对照第 7 课 CPU）：
```
寄存器 < 共享内存(片上,快) < 全局内存(显存, 400~800 周期!!)
```

**"缓存分块"在 GPU 改名"共享内存 tiling"**——同一思想。

### ① 合并内存访问（= 连续访存 / 空间局部性）
一个 warp 的 **32 线程访问连续内存** = 合并(coalesced) → 一次搬一整块。跳步访问 → 32 次单独搬运 → 带宽暴跌。**SoA/行优先**（第 7 课）依旧金科玉律。

### ② 提高计算访存比（= 第 1 课 Roofline 算术强度）
朴素矩阵乘每线程 1 元素：2 次全局读才对 1 次 → 比值 1/3，喂不饱。
→ **每线程算一个 4×4 小块**：比值升到 16/8 → 全局访存次数大降。实测 1019μs → **455μs**。→ `global_memory_optimization.cu`

### ③ 共享内存 tiling（= 循环分块治 L3 Bound）
```c
__shared__ float tileA[32][32], tileB[32][32];
// 线程块协作把全局数据搬进共享内存 → __syncthreads() → 从共享内存反复用
```
全局访存从 O(width) 降到 O(width/32) → 417μs → **256μs**。→ `shared_memory_optimization.cu`

### ④ Bank 冲突（GPU 版的伪共享）
共享内存分 **32 个 bank**；一个 warp 多线程撞**同一 bank 的不同地址** → 串行化（本质和 CPU 伪共享同理：都想挤同一物理通道）。消除后共享内存效率 **21%→91%**。

### ⑤ 缓存小知识
GPU **只有"读"会被缓存，"写"不缓存** → 转置时"非合并读(走缓存) + 合并写"比反过来快（transpose1 41μs → transpose2 **19.5μs**）。

---

## 五、掩盖延迟：预取 + 展开

**双缓冲预取**：全局访存 400~800 周期、算术 0~20 → 必须**边算这次的、边取下次的**。开两块共享内存 `tile[2][32][32]` 乒乓切换。实测 256→**237μs**。→ `data_prefetch.cu`
> 这是第 7 课 `__builtin_prefetch` 的**重型加强版**——CPU 靠硬件预取器，GPU 常得你**手动双缓冲**。

**循环展开**：GPU 靠"运算指令填满流水线"提并行，且没分支预测。归约最后降到 **1 个 warp（32 线程）**时同束天然同步，`__syncthreads()` 降级为 **`__syncwarp()`** → 168μs。→ `loop_unroll.cu`

**归约优化接力赛**（把几招全叠上）：
```
相邻配对 393 → 交错配对 227 → 共享内存 247 → 消 bank 冲突 217 → 循环展开 168 μs
```

---

## 六、把 GPU 概念"接回"前面 CPU 知识（本课最高价值）

| GPU 概念 | 其实就是前面某课的化身 |
|----------|---------------------|
| warp / SIMT | 第 6 课 SIMD，只是 32 路 |
| **warp 分化** | 第 5 课"分支优化治 Bad Speculation"，后果放大 32× |
| **合并访存** | 第 7 课"连续访问/空间局部性" |
| **共享内存 tiling** | 第 7 课"缓存分块"（同思想换硬件）|
| **计算访存比** | 第 1 课 **Roofline 算术强度**（每搬 1 字节能算几次）|
| **数据预取双缓冲** | 第 7 课 `__builtin_prefetch` 的加强 |
| 循环展开 | 第 6 课 ILP 展开 + 寄存器压力代价 |

> **一句话总纲**：**GPU 不追求"单线程快"，而追求"吞吐 + 掩盖延迟"**。所有手法归结为——**让 warp 整齐地合并访存（喂饱带宽）+ 让计算和取数/多线程重叠（藏住延迟）**。

---

## 七、代码实战 · 本机实测（RTX 5070 Ti, CUDA 13.4, sm_120）

**设备**：NVIDIA GeForce RTX 5070 Ti · **70 个 SM** · warp=32 · 计算能力 **12.0 (Blackwell)** · 15838 MB 显存。

**实验 ⑨ 三级演进的矩阵乘**（N=1024 单精度，`cudaEvent` 计时，5 次取最小）：

| 版本 | 时间 | 实测算力 | vs ① 加速 |
|------|------|---------|----------|
| ① Naive 1D block=(1024,1) 极端一维 | 1.222 ms | **1757 GFLOPS** | 1.00x |
| ② Naive 2D block=(32,32) 合理二维 | 0.708 ms | 3032 GFLOPS | **1.73x** |
| ③ **Shared memory tiling (TILE=32)** | **0.552 ms** | **3887 GFLOPS** | **2.21x** 🔥 |

**读数据**：
- ①→②：只是**把线程从 1D 摆成 2D 32×32**（warp 对齐 + 更易 coalesced）→ **1.73x**，**代码一行没改、内存访问模式相同**。→ 印证 §二「布局即性能」。
- ②→③：**共享内存 tiling 是真正的杀招**——把 `O(N)` 次全局读降到 `O(N/32)` 次，每 block 用 `__shared__` tile 缓存 A/B 的 32×32 小块。再快 **1.28x**。→ 印证 §四「共享内存 tiling 就是第 7 课缓存分块的 GPU 版」。

**核心 benchmark 代码骨架**（可直接保存复现）：
```c
// ② 合理 2D block: 每线程算 C 一个元素
__global__ void mm_2d(const float*A,const float*B,float*C){
  int col=blockIdx.x*blockDim.x+threadIdx.x, row=blockIdx.y*blockDim.y+threadIdx.y;
  if(row>=N||col>=N) return;
  float s=0; for(int k=0;k<N;k++) s+=A[row*N+k]*B[k*N+col];
  C[row*N+col]=s;
}
// ③ 共享内存 tiling (TILE=32)
__global__ void mm_sh(const float*A,const float*B,float*C){
  __shared__ float As[32][32],Bs[32][32];
  int col=blockIdx.x*32+threadIdx.x, row=blockIdx.y*32+threadIdx.y;
  float s=0;
  for(int t=0;t<N/32;t++){
    As[threadIdx.y][threadIdx.x]=A[row*N+t*32+threadIdx.x];
    Bs[threadIdx.y][threadIdx.x]=B[(t*32+threadIdx.y)*N+col];
    __syncthreads();
    #pragma unroll
    for(int k=0;k<32;k++) s+=As[threadIdx.y][k]*Bs[k][threadIdx.x];
    __syncthreads();
  }
  C[row*N+col]=s;
}
// 启动:  dim3 block(32,32), grid((N+31)/32,(N+31)/32);
//        mm_sh<<<grid,block>>>(Ad,Bd,Cd);
```

**编译运行**：
```bash
nvcc -O3 -arch=native cuda_bench.cu -o cuda_bench
./cuda_bench
```

> ⚠️ CUDA 13.4 里 `cudaDeviceProp.clockRate`/`l1CacheSize` 已**移除**，用 `p.major/minor/multiProcessorCount` 才编得过。

**实验 ⑨b · Warp 分化的“经典结论”在 Blackwell 上失效** 🤯：

同机（RTX 5070 Ti）重跑工程文档 §三 里的归约对比：

| 版本 | 4M 数据 | 16M 数据 | 工程文档 RTX 3090 |
|------|---------|----------|----------------|
| ① 相邻配对 (经典“有 warp 分化”) | 0.068 ms | **0.317 ms** ✅ 更快 | 393 μs |
| ② 交错配对 (经典“消分化”) | 0.076 ms | 0.347 ms | 227 μs (快 1.7x) |

**交错反而慢 9%！** 工程文档里的“1.7x”在当代 GPU 上完全相反。三个原因：
1. **现代 GPU 分歧代价大幅下降**（Fermi 时 predication 很贵，Blackwell 对分支处理好了很多）
2. **交错版 `sdata[2*stride*tid]` 引入 shared memory 跳步 access** → 潜在 **bank conflict**（§四 ④ 讲的那个），相抵了收益
3. 全局 load 主导时间，两种归约 phase 都不构成瓶颈

> 🎯 **元教训**：代际差异会直接推翻“教科书结论”。**工程文档里那个 1.7x 属于历史参考，你在新硬件上要自己重测**（详见 `30-测量纪律手册` 元教训 3 + 陷阱 13）。**交错配对不是万能药**，它是拿 bank conflict 换 divergence——**消除一种瓶颈可能引入另一种**，这就是第 20/21 号复盘里“药箱分类”的具体演绎。

**未跑但可下轮补**：双缓冲预取——`basis/CUDA/DataPrefetch/data_prefetch.cu` 里现成代码，本机直接可验证。

---

## 八、一句话收口

> **思维转换**：CPU 求低延迟、GPU 求高吞吐+掩盖延迟。**线程三层铺满 SM**（32×32 二维布局忌 (1,1024)）、**warp 分化是头号隐形杀手**（交错配对替代相邻）、**访存四板斧**（合并 / 计算访存比 / 共享内存 tiling / 消 bank 冲突）、**双缓冲+展开藏延迟**。全是前 7 课思想的"GPU 极限版"。

## 九、延伸阅读

- 原始详细文档：[`docs/cuda_optimization.md`](../theory/cuda_optimization.md)
- 相关代码：[`basis/CUDA/`](../../basis/CUDA/)
- 承接：**`10-MPI优化.md`**（把"多楼层独立厨房靠传菜窗口"的思维补齐）
