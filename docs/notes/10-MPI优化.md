# 10 · MPI 程序优化

> **立意**：并行三范式收官。**MPI 是分布式内存**——每个进程**有自己独立的内存**，别的进程数据看不到摸不着，想要只能**发消息**。于是催生前 9 课从没出现过的 BOSS——**通信（Communication）**。**MPI 优化的全部艺术，就是"和通信搏斗"：少通信、早通信、把通信藏进计算里。**

---

## 一、MPI 编程模型：每个进程"我是谁、有几个人"

MPI 是接口标准（实现有 OpenMPI / MPICH）。核心：**每个进程跑同样的代码，靠 `rank` 区分身份**：
```c
MPI_Init(&argc,&argv);
MPI_Comm_rank(comm,&rank);   // 我是几号进程
MPI_Comm_size(comm,&size);   // 一共几个
... 按 rank 分工 ...
MPI_Finalize();
```
编译运行：`mpicc -o app app.c && mpirun -np 4 app`。→ [`mpi_hello.c`](../../basis/MPI/Basics/mpi_hello.c)

**通信函数两大类**：

| 类别 | 函数 | 用途 |
|------|------|------|
| **点对点** | `MPI_Send`/`Recv`（阻塞）、`MPI_Isend`/`Irecv`（**非阻塞**）| 指定进程间收发 |
| **集合通信** | `Bcast` 广播 / `Scatter` 分发 / `Gather` 收集 / `Reduce` 归约 / `Barrier` 全局同步 | 一对多 / 多对一 / 全体 |

### ⭐ 关键成本模型：MPI 版的"存储层次"
$$T_{comm} = \alpha + \beta \cdot n$$
- **α 启动延迟**（固定，与数据量无关）
- **β·n 传输时间**（∝消息大小 n）

> **推论**：**1 个大消息 ≪ N 个小消息**（每次都要付 α）→ **"合并小消息"是黄金法则**（与第 7 课"批量 I/O"、第 2 课"减少系统调用"一脉相承）。

---

## 二、数据划分：决定通信量（`DataPartitioning/`）

矩阵乘为例，基础版把 A、B **整个广播**到每进程 → 通信爆炸。三种更省的划分：

| 划分 | 每进程存 | 通信 | 缓存 | 文件 |
|------|---------|------|------|------|
| **按行分解** | A 的几行 + 完整 B | 广播 B、`Scatter` A、`Gather` C | 一般 | `row_partition.c` |
| **按列分解** | A 几列 + B 几行 | 各算部分 C，`Reduce` 求和 | 一般 | `column_partition.c` |
| **棋盘式(Cannon)** ⭐ | 一个 √P×√P 小块 | **只和邻块交换** | **最高** | `cannon_algorithm_blocking.c` |

**Cannon 算法**（重点）：进程排成**二维网格**(`MPI_Cart_create`)，每进程只持一块，靠 **A 左移、B 上移的循环移位**（`MPI_Sendrecv_replace`）把需要的块"轮到"手边。
> 👉 **本质就是第 7 课"缓存分块"的进程版**：把工作集切小、只跟邻居通信 → 通信量从 O(N²) 降到 O(N^{3/2}/√P)。**"通信量" vs "并行度"的经典权衡**。

---

## 三、⭐ 重叠通信与计算：MPI 头号提速招（`OverlapCommunication/`）

**阻塞通信**下，进程发完就干等：
```
通信 → 计算 → 通信 → 计算    (通信时计算单元闲置 = 浪费)
```
**非阻塞通信**(`Isend`/`Irecv`) + **双缓冲**，让"这次的计算"和"下次的通信"**同时**进行：
```
[计算K ‖ 通信K+1] → [计算K+1 ‖ 通信K+2]
```
套路：开两块缓冲区乒乓——算 A 块时，让网络在后台传 B 块，`MPI_Wait/Test` 确认到位再切。→ `cannon_algorithm_nonblocking.c`

> 🔄 **这就是第 9 课 CUDA"双缓冲数据预取"的同一招**——CUDA 藏**显存延迟**，MPI 藏**网络延迟**。掩盖延迟的手法跨越了硬件层级。

---

## 四、负载均衡：分不均，多核反而更慢（`LoadBalancing/`）

**素数筛反例**（极深刻的教材）：
- **交叉分解**（进程轮流取 1,5,9…）：筛到某步**只有少数进程有活干、其余干等** → 实测**并行反而比串行还慢**！→ `prime_interleaved.c`
- **按块分解**（各取连续一大段）：负载均衡，`MPI_Gatherv` 收不等长结果 → `prime_block.c`

> 👉 呼应第 1 课 **Amdahl**：木桶效应——并行效率被"最闲的进程"拖死。也呼应第 8 课 OpenMP 的 `schedule`。

---

## 五、冗余计算减少通信：用"算"换"传"（`RedundantComputation/`）

反直觉但常用：**当通信比计算还贵时，宁让每个进程多算一点，也少发消息**。
- 筛法例：与其反复通信拿小素数表，不如**每进程独立本地生成 √n 内的小素数表** → `prime_redundant.c`
- 或**扩展本地筛选范围**（多筛相邻 margin）换"少跨进程取数"

> **本质**：在第 1 课 Roofline 语言里，这是**主动降低对"通信算术强度"的依赖**——用廉价算力换昂贵带宽。适用：**通信密集、带宽受限、延迟敏感**。核心是找"计算成本 vs 通信成本"的平衡点。

---

## 六、性能指标：把第 1 课理论在这里落地

- **加速比** S = T_serial / T_parallel
- **效率** E = S / P（越接近 1 越好）
- **可扩展性**：
  - **强扩展**：问题规模固定、加进程 → 呼应 **Amdahl**（串行部分是天花板）
  - **弱扩展**：问题和进程**同步放大** → 呼应 **Gustafson**（MPI/超算的主场！）

> 到这儿第 1 课"两个定律"的实战意义才真正闭环：**单机多核(OpenMP) 看 Amdahl，跨节点集群(MPI) 靠 Gustafson 摊大饼**。

---

## 七、常见陷阱（工程血泪）

1. **死锁 Deadlock** ⭐：两进程都先 `Send` 再 `Recv`，缓冲区满时**互相死等**。→ 解法：**`MPI_Sendrecv`/`MPI_Sendrecv_replace`**（原子收发，Cannon 就靠它）、或一方先发后收。
2. **缓冲区不匹配**：`Recv` 声明的接收数 < 实际发来 → 溢出/截断。
3. **tag 混乱**：同 tag 收错消息 → 用明确 tag 常量区分。
4. **不检查返回值**：MPI 函数都应查 `!= MPI_SUCCESS`。
5. **资源泄漏**：`MPI_Comm_dup` 后要 `MPI_Comm_free`。
6. **混合编程(MPI+OpenMP)**：用 `MPI_Init_thread(...MPI_THREAD_MULTIPLE)` 协商线程级别。

**调优工具**：`MPIP` / `TAU` / `Score-P` / Intel Trace Analyzer——第 2 课"追踪类"工具在超算领域的化身。

---

## 八、三模型对照总表（本课最实用的收口）

| | **OpenMP**(第 8 课) | **CUDA**(第 9 课) | **MPI**(本课) |
|---|---|---|---|
| 内存模型 | 共享 | 共享(显存)+片上 | **分布式** |
| 并行单位 | 线程(几十~几百) | 线程(上万,warp) | 进程(几~几万) |
| 主要开销 | fork-join / 同步 / 负载 | **访存带宽**+延迟 | **通信**(α+β·n) |
| 王牌手法 | schedule / reduction / nowait | 合并访存 / 共享内存 / 双缓冲 | 非阻塞双缓冲 / 数据划分 / 冗余计算 |
| 适用 | 单机多核 | GPU 数据并行 | 跨节点集群 / 超算 |
| 理论呼应 | Amdahl | Roofline 算术强度 | Gustafson 弱扩展 |

---

## 九、代码实战 · 本机实测（Intel MPI 2021.18, 单机 24 核）

### 启动备忘 ⚠️
本机 sandbox 环境下 `mpirun` 默认走 `hydra_bstrap_proxy` 会失败（helper 进程无法孵化）。**绕过方式**：
```bash
# 不用 mpirun, 直接:
mpiexec.hydra -bootstrap ssh -n N ./app
# 前置: ssh localhost 可无密登录
```

### 实验 ⑩ 加速比 + α+β·n 通信成本

**A. 矩阵乘按行分解 (N=640, 单精度) · 每 rank 分 N/size 行 · Bcast+Gather 完整流程 · 5 次取 min**：

| size | 时间 | 加速比 | 效率 E=S/P |
|------|------|--------|-----------|
| 1 | 0.4862 s | 1.00x | 100% |
| 2 | 0.2485 s | 1.96x | 98% |
| 4 | 0.1246 s | 3.90x | 98% |
| 8 | 0.0706 s | **6.89x** | 86% |

**读曲线**：
- **单机内 loopback 通信极快**，加速比一路接近线性（8 进程 6.89x，仍 86% 效率）→ 印证 **OpenMP/CUDA/MPI 三者本质都是 Amdahl，只是通信代价不同**。
- 效率随 size 递减（100→98→98→86%）：**每多一个 rank，Bcast/Gather 就多一笔 α 启动延迟**——这就是 MPI 版 Amdahl 的表现。
- 真实集群跨节点（α 大 100~1000 倍）时，这条曲线会**更早封顶**——正是 §三「非阻塞双缓冲重叠通信」和 §二「Cannon 棋盘只和邻居通信」这两味药的存在意义。

**B. ⭐ α+β·n 通信成本模型直接实测**（rank 0 → rank 1，各 1000 轮，总字节相同）：

| 消息模式 | 消息数 | 总字节 | 耗时 | vs 大消息 |
|---------|-------|--------|------|----------|
| **8×1KB 小消息** × 1000 | 8000 | 8 MB | 0.0044~0.0062 s | 慢 |
| **1×8KB 大消息** × 1000 | 1000 | 8 MB | 0.0011~0.0013 s | **快 4~5x** 🔥 |

> **同样 8 MB 数据，只是把消息数从 8000 降到 1000 → 快 4~5 倍**。这就是 **T = α + β·n** 里 α 项的直观体现——**每次 Send 都要付一笔固定启动延迟**，数据量再小也省不掉。**「合并小消息」不是玄学，是能被本机量化的物理事实**。

### 关键代码骨架
```c
// 按行分解矩阵乘 (每 rank 拿自己那几行, 全部结果 gather 到 rank0)
static void row_matmul(int rank,int size){
  int rows=N/size, start=rank*rows;
  MPI_Bcast(A, N*N, MPI_FLOAT, 0, MPI_COMM_WORLD);
  MPI_Bcast(B, N*N, MPI_FLOAT, 0, MPI_COMM_WORLD);
  for(int i=0;i<rows;i++)
    for(int j=0;j<N;j++){
      float s=0; int gi=start+i;
      for(int k=0;k<N;k++) s+=A[gi*N+k]*B[k*N+j];
      Cl[i*N+j]=s;
    }
  MPI_Gather(Cl, rows*N, MPI_FLOAT, C, rows*N, MPI_FLOAT, 0, MPI_COMM_WORLD);
}
// α+β·n 对比: 用 MPI_Ssend (同步发) 而不是 MPI_Send, 否则小消息会被内部缓冲吃掉
for(int i=0;i<ITER;i++) for(int j=0;j<8;j++) MPI_Ssend(buf,1024,MPI_CHAR,1,j,MPI_COMM_WORLD);
for(int i=0;i<ITER;i++)                      MPI_Ssend(buf,8192,MPI_CHAR,1,0,MPI_COMM_WORLD);
```

**⚠️ 三个踩过的坑**（都进了 `30 测量纪律手册`）：
1. **`MPI_Gather` sendbuf 和 recvbuf 不能重叠**（同一 C 数组不行，会 "Buffers must not be aliased"）→ 用独立 `Cl` 装每 rank 局部结果。
2. **`MPI_Reduce` 是集合通信，所有 rank 必须一起调**——只在 rank0 调会死锁 → 简单起见改每 rank 自己 printf。
3. **`MPI_Send` 对小消息会内部缓冲**（异步、立即返回）→ 测 α 必须用 **`MPI_Ssend`（同步发送）**，否则测不到启动延迟。

### 实验 ⑩b · Scatter vs Bcast “减通信量”在单机上失效 🤐

理论上 `MPI_Scatter` 只给每 rank 发 `N²/size` 字节，而 `MPI_Bcast` 强制每 rank 收全量 `N²` 字节——**字节数理论上少 size 倍**。但本机实测：

| 场景 | A. 双 Bcast | B. Scatter + Bcast | 差 |
|------|---------|-------------------|-----|
| N=1024, size=8 | 0.2987 s | 0.3008 s | 一0.7% (Scatter 反而慢) |
| N=2048, size=8 | 3.9015 s | 3.8421 s | **快 1.5%** (基本看不出) |

**为什么几乎无差别？** 两个量级对拼：
- **计算 8 GB ops** (N=2048³/8 ≈ 1G FLOP per rank × 8 = 8G) → **3.8 s**
- **通信 16 MB** 量级 → loopback 上 只 **~ms**
- 即使 Scatter 把通信量降 8 倍，**从几 ms 降到 <1 ms** → 对 3.8s 总时间无影响

> 🎯 **元教训**：**“减通信量”的优化只在通信真成为瓶颈时才有收益**。单机 loopback 上你测不出 Cannon/Scatter 的价值——**它们是为跨节点集群（α 大 100~1000 倍）设计的**。这反向验证了 §二 Cannon 、§三 非阻塞重叠的适用前提：**先确认通信就是瓶颈，再开那些药**。
> 下次你在真集群上重跑这个 ⑩b，结果会反转。**这就是 `30` 手册元教训 3 的又一次坐实**。

---

## 十、一句话收口

> **思维跳变**：MPI 是**分布式内存**，进程间**只能显式传消息** → 催生**通信**这个 BOSS。**通信成本 = α + β·n** → 合并小消息、少通信是铁律。三大提速武器：**数据划分**（行/列/Cannon 棋盘，降通信量）、**非阻塞双缓冲重叠通信与计算**（藏延迟）、**负载均衡**（否则比串行还慢）；进阶：**冗余计算换通信**。**并行绝不是免费午餐**——每个 `MPI_Sendrecv` 都要算"收益 vs 协调开销"。

## 十一、延伸阅读

- 原始详细文档：[`docs/mpi_optimization.md`](../theory/mpi_optimization.md)
- 相关代码：[`basis/MPI/`](../../basis/MPI/)
- 承接：**`21-并行优化全景复盘.md`**（把 OpenMP/CUDA/MPI 缝成一张图）
