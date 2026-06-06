# MPI 程序优化

## 1. MPI 编程简介

### 1.1 什么是 MPI

MPI（Message Passing Interface）是一组用于编写并行程序的多节点数据通信标准。

- 通常基于单核处理器编写的程序无法直接利用多核处理器，因此可以手动将串行程序改写为并行程序以充分利用多核处理器
- MPI 作为一种服务于进程间通信的消息传递编程模型，可运行在不同的机器或平台上，具有很好的可移植性
- 可以将 MPI 理解为一种协议或接口，而 OpenMPI 及 MPICH 是这一接口的常用实现

### 1.2 MPI 函数库

MPI 的库函数大体上分为五类：基本函数、阻塞型点对点传递函数、非阻塞型点对点传递函数、组消息传递函数、MPI 自定义数据类型函数。

#### 基本函数

主要用于 MPI 环境的初始化、资源释放、获取程序信息和执行时间等。

```c
int MPI_Init(int *argc, char **argv[]);                // 初始化 MPI 环境
int MPI_Finalize(void);                                 // 终止 MPI 执行环境
int MPI_Comm_rank(MPI_Comm comm, int *rank);            // 获得当前进程标识
int MPI_Comm_size(MPI_Comm comm, int *size);            // 获取通信域包含的进程总数
int MPI_Get_processor_name(char *name, int *resultlen); // 获得本进程的机器名
double MPI_Wtime(void);                                 // 以秒为单位返回执行时间
```

#### 阻塞型点对点传递函数

需要等待指定操作实际完成，或至少所涉及的数据已被 MPI 系统安全备份后才返回。

```c
// 消息发送：将发送缓冲区 buf 中 count 个 datatype 类型的数据发送到标识号为 dest 的目的进程
int MPI_Send(void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm);

// 消息接收：从标识号为 source 的源进程接收 count 个 datatype 类型的数据到缓冲区 buf 中
int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
             MPI_Comm comm, MPI_Status *status);
```

#### 非阻塞型点对点传递函数

调用总是立即返回，实际操作由 MPI 系统在后台进行。使用非阻塞会带来性能提升，但增加了编程难度。

```c
// 非阻塞发送：比阻塞操作多一个 MPI_Request *request 参数
int MPI_Isend(void *buf, int count, MPI_Datatype datatype, int dest, int tag,
              MPI_Comm comm, MPI_Request *request);

// 非阻塞接收
int MPI_Irecv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
              MPI_Comm comm, MPI_Request *request);

// 等待发送或接收结束然后返回
int MPI_Wait(MPI_Request *request, MPI_Status *status);

// 若 flag 为 true 则如同执行了 MPI_Wait；若为 false 则如同执行了空操作
int MPI_Test(MPI_Request *request, int *flag, MPI_Status *status);

// 阻塞式检查
int MPI_Probe(int source, int tag, MPI_Comm comm, MPI_Status *status);

// 非阻塞式检查
int MPI_Iprobe(int source, int tag, MPI_Comm comm, int *flag, MPI_Status *status);
```

#### 集合通信函数

集合通信调用可以和点对点通信共用一个通信域，MPI 保证由集合通信产生的消息不会与点对点通信产生的消息相混淆。

```c
// 障碍同步：阻塞直到所有进程组成员都调用了它
int MPI_Barrier(MPI_Comm comm);

// 广播：从 root 进程将消息广播发送到组内所有进程
int MPI_Bcast(void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm);

// 收集：每个进程将发送缓冲区内容发送到根进程，按进程序列号依次存放
int MPI_Gather(void *sendbuf, int sendcount, MPI_Datatype sendtype,
               void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm);

// 散播：从根进程部分地散播缓冲区中的值到进程组
int MPI_Scatter(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm);

// 归约：将组内每个进程输入缓冲区中的数据按 op 操作组合，结果返回到 root 进程
int MPI_Reduce(void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
               MPI_Op op, int root, MPI_Comm comm);
```

#### 自定义数据类型函数

可以有效减少消息传递次数，增大通信力度，同时避免或减少数据在内存中的拷贝。

```c
// 连续数据类型生成
int MPI_Type_contiguous(int count, MPI_Datatype oldtype, MPI_Datatype *newtype);

// 向量数据类型生成
int MPI_Type_vector(int count, int blocklength, int stride, MPI_Datatype oldtype, MPI_Datatype *newtype);

// 索引数据类型生成
int MPI_Type_indexed(int count, int *array_of_blocklengths, MPI_Aint *array_of_displacements,
                     MPI_Datatype oldtype, MPI_Datatype *newtype);

// 结构数据类型生成
int MPI_Type_struct(int count, int *array_of_blocklengths, MPI_Aint *array_of_displacements,
                    MPI_Datatype array_of_types, MPI_Datatype *newtype);

// 数据类型注册
int MPI_Type_commit(MPI_Datatype *datatype);

// 数据类型释放
int MPI_Type_free(MPI_Datatype *datatype);
```

### 1.3 MPI 程序编写

MPI 程序由头文件、变量声明、程序开始（初始化）、计算与通信、程序结束五部分组成。

```c
#include <stdio.h>
#include <mpi.h>

int main(int argc, char *argv[]) {
    int myid, data;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &myid);
    if (myid == 0) {
        data = 666;
    }
    MPI_Bcast(&data, 1, MPI_INT, 0, MPI_COMM_WORLD);
    printf("进程 %d 成功接收到数据 %d\n", myid, data);
    MPI_Finalize();
    return 0;
}
```

#### 入门示例：点对点通信

使用 0 号进程发送一个整型数据，1 号进程接收。

```c
#include <stdio.h>
#include <mpi.h>

int main(int argc, char *argv[]) {
    int world_rank, world_size, send, recv;
    MPI_Status status;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    if (world_rank == 0) {
        send = 666;
        MPI_Send(&send, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
        printf("共 %d 个进程，其中进程 %d 成功发送数据 %d\n", world_size, world_rank, send);
    }
    if (world_rank == 1) {
        MPI_Recv(&recv, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
        printf("共 %d 个进程，其中进程 %d 成功接收数据 %d\n", world_size, world_rank, recv);
    }
    MPI_Finalize();
    return 0;
}
```

**编译与运行：**

```bash
mpicc -o ex ex.c
mpirun -np 2 ex
```

**输出：**

```
共 2 个进程，其中进程 0 成功发送数据 666
共 2 个进程，其中进程 1 成功接收数据 666
```

### 1.4 MPI 版矩阵乘法

#### 串行版本

```c
#include <stdio.h>
#include <mpi.h>
#include <time.h>
#include "mympi.h"
#define DIMS 1000

int main(int argc, char *argv[]) {
    data_t *A, *B, *C;
    double start_time, end_time;
    A = (data_t*)malloc(sizeof(data_t) * DIMS * DIMS);
    B = (data_t*)malloc(sizeof(data_t) * DIMS * DIMS);
    C = (data_t*)malloc(sizeof(data_t) * DIMS * DIMS);

    Init_Matrix(A, DIMS * DIMS, 2);  // 随机生成 0/1 矩阵
    Init_Matrix(B, DIMS * DIMS, 2);
    Init_Matrix(C, DIMS * DIMS, 1);  // 生成 0 矩阵

    start_time = (double)clock();
    Mul_Matrix(A, B, C, DIMS, DIMS, DIMS);
    end_time = (double)clock();
    printf("执行时间: %.2lf ms\n", (end_time - start_time) / 1e3);

    free(A); free(B); free(C);
    return 0;
}
```

#### 基础并行版本

使用 `MPI_Bcast` 将矩阵广播到所有进程，每个进程计算结果矩阵的一部分行。

```c
#include <stdio.h>
#include <mpi.h>
#include "mympi.h"
#define DIMS 1000

int main(int argc, char *argv[]) {
    data_t *A, *B, *C;
    int world_rank, world_size, lens, i;
    double start_time, end_time;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    if (DIMS % world_size != 0) {
        printf("总进程数 world_size 应整除矩阵维数 DIMS!\n");
        MPI_Finalize();
        return 0;
    }

    // 为所有进程创建 A、B、C 的空间并初始化
    A = malloc(sizeof(data_t) * DIMS * DIMS);
    B = malloc(sizeof(data_t) * DIMS * DIMS);
    C = malloc(sizeof(data_t) * DIMS * DIMS);
    Init_Matrix(C, DIMS * DIMS, 1);
    if (world_rank == 0) {
        Init_Matrix(A, DIMS * DIMS, 2);
        Init_Matrix(B, DIMS * DIMS, 2);
    }

    start_time = MPI_Wtime();

    // 广播矩阵 A、B 到所有进程
    MPI_Bcast(A, DIMS * DIMS, MPI_FLOAT, 0, MPI_COMM_WORLD);
    MPI_Bcast(B, DIMS * DIMS, MPI_FLOAT, 0, MPI_COMM_WORLD);

    // 每个进程要处理的 A 的行数
    lens = DIMS / world_size;

    // 将 A 对应行与 B 相乘，结果存于 C 对应行
    Mul_Matrix(A + lens * DIMS * world_rank, B, C + lens * DIMS * world_rank, lens, DIMS, DIMS);

    // 各进程将自身计算的 C 广播到其它进程
    for (i = 0; i < world_size; i++) {
        MPI_Bcast(C + i * lens * DIMS, lens * DIMS, MPI_FLOAT, i, MPI_COMM_WORLD);
    }

    end_time = MPI_Wtime();
    printf("进程 %d 的运行时间: %lf s\n", world_rank, (end_time - start_time));

    free(A); free(B); free(C);
    MPI_Finalize();
    return 0;
}
```

---

## 2. 数据划分优化

数据划分通常对规模较大的数据进行划分，将分解后的数据块映射到多个处理器上，实现在多个进程上同时执行以加快程序运行速度。在保证结果正确的前提下，要使数据划分后程序性能较好，就需要使负载尽可能保持均衡。

以矩阵乘法为例，基础并行算法使用 0 号进程将矩阵完整广播到各个进程，效率不高。可以采用数据划分方法让不同进程执行矩阵 A 某个分块和矩阵 B 某个分块的乘法计算，得到结果 C 的不同部分，最后聚合得到完整的结果 C。

常用的矩阵划分方法有三种：按行分解、按列分解、棋盘式分解。

### 2.1 按行分解

由于计算矩阵 C 的第 i 行时只需要用到矩阵 A 的第 i 行以及完整的矩阵 B，因此每个进程上存储 A 中多余的行会增加不必要的通信。按行划分方式下，每个进程负责处理矩阵 A 的若干行与矩阵 B 相乘，得到结果 C 中的若干行，最后合并结果。

**实现流程：**

1. 由 0 进程生成矩阵 A 和 B
2. 0 号进程将矩阵 B 发送到所有进程
3. 0 号进程依据总进程数与矩阵维数的关系划分任务，分别将 A 的对应若干行发送给不同进程
4. 各个进程完成矩阵 C 部分行的计算
5. 将结果聚合到 0 号进程

**`MPI_Scatter` 函数：**

与 `MPI_Bcast` 类似，是一对多的通信函数，但 `MPI_Scatter` 的 0 号进程向每个进程发送的数据可以不同。0 号进程将连续的数据按照进程号大小的顺序依次发送给通信域中的所有进程。

```c
int MPI_Scatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                void *recvbuf, int recvcount, MPI_Datatype recvtype,
                int root, MPI_Comm comm)
```

| 参数 | 说明 |
|------|------|
| `sendbuf` | 发送消息缓冲区的起始地址（仅根进程有意义） |
| `sendcount` | 发送给每个进程的数据个数 |
| `sendtype` | 发送的数据类型 |
| `recvbuf` | 接收缓冲区的起始地址 |
| `recvcount` | 待接收的元素个数 |
| `recvtype` | 接收类型 |
| `root` | 数据发送进程的序列号 |
| `comm` | 通信域 |

**`MPI_Gather` 函数：**

与 `MPI_Scatter` 相反，是多对一的通信函数。每个进程将一个相同大小的数据块发送给根进程，按进程号大小排序存储到接收缓冲区。

```c
int MPI_Gather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
               void *recvbuf, int recvcount, MPI_Datatype recvtype,
               int root, MPI_Comm comm)
```

| 参数 | 说明 |
|------|------|
| `sendbuf` | 发送缓冲区的起始地址 |
| `sendcount` | 每个进程发送的数据个数 |
| `sendtype` | 发送的数据类型 |
| `recvbuf` | 接收缓冲区的起始地址（仅根进程有意义） |
| `recvcount` | 从每个进程接收到的数据个数 |
| `recvtype` | 接收的数据类型 |
| `root` | 接收进程的进程号 |
| `comm` | 通信域 |

### 2.2 按列分解

按行分解降低了矩阵 A 在每个进程中的存储空间和通信消耗，但没有对 B 矩阵进行处理。按列分解方法可以同时降低 B 矩阵内存开销：将 A 矩阵和 B 矩阵按列划分，每个进程负责处理矩阵 A 的若干列与矩阵 B 的若干行相乘，得到结果 C 中的一部分，最后将各进程的计算结果进行归约操作得到完整的结果矩阵 C。

**实现流程：**

1. 由 0 进程生成矩阵 A 和 B
2. 0 号进程依据总进程数与矩阵维数划分任务，分别将 A 的对应若干列和 B 的对应若干行发送给不同进程
3. 各个进程完成部分矩阵 C 的计算
4. 将各个进程的矩阵 C 汇聚到 0 号进程，并对各 C 矩阵的对应位置进行归约求和操作

**`MPI_Reduce` 函数：**

将组内每个进程输入缓冲区中的数据在相应位置按给定的操作进行运算，结果返回到 0 号进程。

```c
int MPI_Reduce(const void *sendbuf, void *recvbuf, int count,
               MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm)
```

| 参数 | 说明 |
|------|------|
| `sendbuf` | 要进行归约操作的元素的起始地址 |
| `recvbuf` | 存放归约结果的起始地址 |
| `count` | sendbuf 中的数据个数 |
| `datatype` | sendbuf 的元素类型 |
| `op` | 归约操作符 |
| `root` | 根进程的进程号 |
| `comm` | 通信域 |

### 2.3 棋盘式分解

按行分解和按列分解都存在一个问题：随着问题规模的增加，通信量和存储量也会急剧增加，导致缓存命中率下降。棋盘式分解在运算时能够极大提高缓存命中率。

在棋盘式分解中，所有进程构成一个虚拟网格，A 矩阵和 B 矩阵也按照这个网格进行数据划分，每个进程只负责一个块内的矩阵乘法。进程间的关系类似基于二维网格的虚拟拓扑结构。

**优点：**
- 虚拟网格数和进程数一一对应
- 进一步节省存储量和通信总量
- 具有较高的可扩展性

#### Cannon 算法

Cannon 算法是棋盘式分解的典型代表，是一种存储有效的算法。它不是将矩阵完整的行或列进行多播传送，而是有目的地在各行和各列上实施循环位移，降低处理器的总存储要求。

**算法流程：**

1. 将矩阵 A 和 B 分成 √P × √P 个分块，每个分块负责 (n/√P) × (n/√P) 的数据（P 为进程总数，n 为矩阵维数），按行优先顺序映射到 P 个处理器上
2. 将块 A_(i,j) 向左循环移动 i 步，将块 B_(i,j) 向上循环移动 j 步
3. P_(i,j) 执行乘法和加法运算，将块 A_(i,j) 向左循环移动 1 步，块 B_(i,j) 向上循环移动 1 步
4. 重复第 3 步，共执行 √P 次乘法和加法运算及 √P 次循环单步移动

**Cannon 算法使用的 MPI 函数：**

```c
// 创建笛卡尔拓扑
int MPI_Cart_create(MPI_Comm comm_old, int ndims, int *dims, int *periods,
                    int reorder, MPI_Comm *comm_cart);

// 坐标到进程号映射
int MPI_Cart_rank(MPI_Comm comm, int *coords, int *rank);

// 进程号到坐标映射
int MPI_Cart_coords(MPI_Comm comm, int rank, int maxdims, int *coords);

// 获取笛卡尔网格中某维度上距离为 disp 的进程编号
int MPI_Cart_shift(MPI_Comm comm, int direction, int disp,
                   int *rank_source, int *rank_dest);

// 阻塞地交换数据（在同一标识的起始地址处）
int MPI_Sendrecv_replace(void *buf, int count, MPI_Datatype datatype,
                         int dest, int sendtag, int source, int recvtag,
                         MPI_Comm comm, MPI_Status *status);
```

**`MPI_Cart_create` 参数说明：**

| 参数 | 说明 |
|------|------|
| `comm_old` | 输入通信域 |
| `ndims` | 笛卡尔网格的维数 |
| `dims` | 大小为 ndims 的整数数组，定义每一维的进程数 |
| `periods` | 大小为 ndims 的逻辑数组，定义网格的周期性（越界后能否循环） |
| `reorder` | 标识数是否可以重排序 |
| `comm_cart` | 带有新笛卡尔拓扑的通信域 |

**`MPI_Cart_shift` 参数说明：**

| 参数 | 说明 |
|------|------|
| `comm` | 带有笛卡尔结构的通信域 |
| `direction` | 需要平移的坐标维数 |
| `disp` | 偏移量 |
| `rank_source` | 本进程在 direction 维 disp 正方向距离的进程号 |
| `rank_dest` | 本进程在 direction 维 disp 反方向距离的进程号 |

**`MPI_Sendrecv_replace` 参数说明：**

| 参数 | 说明 |
|------|------|
| `buf` | 发送和接收数据的起始地址 |
| `count` | 发送和接收数据的个数 |
| `datatype` | 数据类型 |
| `dest` | 目的进程号 |
| `sendtag` | 发送数据的标识 |
| `source` | 源进程号 |
| `recvtag` | 接收数据的标识 |
| `comm` | 源和目的的通信域 |
| `status` | 发送和接收的状态 |

---

## 3. 重叠通信和计算

### 3.1 基本原理

程序串行执行时需要先完成通信再进行计算，通信和计算都是串行执行的：

```
通信 → 计算 → 通信 → 计算 → 通信 → 计算
```

通信与计算重叠可以提高并行程序的运算速度、避免程序隐式串行化以及使进程间通信的竞争达到最小化：

```
[计算 + 通信] → [计算 + 通信] → [计算 + 通信]
```

### 3.2 实现方法

在方法上，可分为单进程的通信与计算重叠以及多进程间的通信与计算重叠。

**单进程方法：**
- 使用多线程技术，让主线程执行与通信无关的代码时开始一个辅助线程用于传输数据
- 在收发数据时使用非阻塞的通信函数去传输数据

**棋盘式分解的优化：**

将通信替换为非阻塞式通信函数，利用通信和计算重叠优化程序中的通信部分。具体方法：

1. 开辟两个缓冲区
2. 非阻塞通信情况下，矩阵 A 分块和矩阵 B 分块相乘与数据收发分别在两个缓冲区同时进行
3. 进程完成一块缓冲区上的计算后，确认第二块缓冲区上的数据是否完成通信
4. 完成通信后才能继续对第二块缓冲区的数据进行计算，并让第一块缓冲区去交换新的数据

### 3.3 非阻塞通信函数

**`MPI_Isend` — 非阻塞发送：**

```c
int MPI_Isend(void *buf, int count, MPI_Datatype datatype, int dest, int tag,
              MPI_Comm comm, MPI_Request *request)
```

| 参数 | 说明 |
|------|------|
| `buf` | 发送缓冲区的起始地址 |
| `count` | 发送数据的个数 |
| `datatype` | 发送数据的数据类型 |
| `dest` | 目的进程号 |
| `tag` | 消息标志 |
| `comm` | 通信域 |
| `request` | 返回的非阻塞通信对象 |

**`MPI_Irecv` — 非阻塞接收：**

```c
int MPI_Irecv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
              MPI_Comm comm, MPI_Request *request)
```

| 参数 | 说明 |
|------|------|
| `buf` | 接收缓冲区的起始地址 |
| `count` | 接收数据的最大个数 |
| `datatype` | 每个数据的数据类型 |
| `source` | 源进程标识 |
| `tag` | 消息标志 |
| `comm` | 通信域 |
| `request` | 非阻塞通信对象 |

**`MPI_Wait` — 等待通信完成：**

```c
int MPI_Wait(MPI_Request *request, MPI_Status *status)
```

**`MPI_Test` — 查询通信完成状态：**

```c
int MPI_Test(MPI_Request *request, int *flag, MPI_Status *status)
```

`flag` 为 0 时代表未完成，为 1 时代表完成。

---

## 4. 负载均衡优化

### 4.1 Eratosthenes 筛法

Eratosthenes 筛法是一种素数求解算法，用于找出一定范围内所有的素数。

**算法步骤：**

1. 假设有一整数列表 0, 1, 2, 3, ..., n，其中的数都未被标记
2. 令 k 等于列表中下一个未被标记的数，将其所有倍数标记
3. 重复第 2 步直到 k² > n
4. 列表中未被标记的数即为所有素数

### 4.2 交叉分解

交叉数据分解是每个进程按照进程号的大小依次进行数据划分。对于给定的数组下标，很容易确定负责该数据计算的进程号。

**问题分析：** 经测试，交叉数据分解的素数筛法相比串行算法性能反而下降。主要原因：

1. 在标记某个数的倍数时，需要重复地计算当前下标所对应的数
2. 数据分布不均匀，某些时刻可能只有一个进程在工作，其它进程都在等待
3. 每次选择下一个未被标记数时都需要进行一次归约操作和广播操作

> **结论**：在严重的负载不均衡情况下，使用多个处理器的效率可能远不及单处理器。使用 MPI 实现程序并行时，需要注意优化数据划分方法使进程负载尽可能均衡。

### 4.3 按块分解

对于 p 个进程，按块分解将原始任务依次划分成 p 个块：
- 若任务量 n 能被 p 整除：每个块大小完全相等
- 若不能整除：前 n%p 个进程处理 ⌈n/p⌉ 个数据，剩余进程处理 ⌊n/p⌋ 个数据

之后每个进程并行地进行筛选。

---

## 5. 冗余计算减少通信

按块分解的并行 Eratosthenes 筛法通信耗时较长。MPI 程序中进程间通信都会产生创建发送或接收消息的开销，因此尽量减少进程间交换的消息数量是有必要的。

当通信时间较长时，进程间相互通信获取数据的时间可能比进程直接计算的时间更长，导致性能下降。为了减少算法中的通信耗时，可以尝试利用冗余计算的方式优化——即让进程多做一些本地计算，以减少需要通信的数据量。

---

## 总结

| 优化技术 | 核心思想 |
|----------|----------|
| 数据划分优化 | 通过按行、按列、棋盘式等分解方式，将数据分布到多个进程并行处理 |
| 重叠通信和计算 | 使用非阻塞通信函数，使通信与计算并行执行 |
| 负载均衡优化 | 合理划分数据块，避免部分进程空闲等待 |
| 冗余计算减少通信 | 以额外的本地计算换取更少的进程间通信 |
