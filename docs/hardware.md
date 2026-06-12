# 硬件配置与调优

> 通过修改系统配置获得性能提升，了解硬件可以更详细地进行性能评估

- [硬件配置与调优](#硬件配置与调优)
  - [处理器](#处理器)
    - [查看处理器相关信息](#查看处理器相关信息)
    - [参数调整](#参数调整)
  - [内存](#内存)
    - [查看内存相关信息](#查看内存相关信息)
    - [参数调整](#参数调整-1)
  - [文件系统](#文件系统)
  - [磁盘](#磁盘)
  - [网络](#网络)

---

## 处理器

**查看处理器相关信息**

```shell
# CPU 架构概览
$ lscpu

# 详细 CPU 信息（每个逻辑核的详细信息）
$ cat /proc/cpuinfo

# 逻辑处理器数量
$ nproc

# 以表格形式显示 CPU 信息
$ lscpu -e

# 查看 CPU 频率信息
$ cat /proc/cpuinfo | grep "cpu MHz"

# 查看 CPU 缓存信息
$ lscpu | grep -i cache

# 查看 NUMA 拓扑
$ lscpu | grep -i numa

# 更详细的硬件信息（需要 root）
$ sudo dmidecode -t processor

# 查看系统物理处理器的个数
$ cat /proc/cpuinfo | grep "physical id" | sort | uniq | wc -l

# 查看每个物理处理器中核的个数
$ cat /proc/cpuinfo | grep "cpu cores"

# 查看系统所有逻辑处理器个数
$ cat /proc/cpuinfo | grep "processor" | wc -l
```

**参数调整**

- 进程优先级调整

  ```shell
  # nice: 启动程序时设置优先级（-20 到 19，值越小优先级越高）
  $ nice -n 10 ./benchmark          # 以较低优先级运行
  $ nice -n -10 ./benchmark         # 以较高优先级运行（需要 root）

  # renice: 调整已运行进程的优先级
  $ renice -n 5 -p 1234             # 将 PID 为 1234 的进程优先级设为 5
  $ renice -n -5 -p 1234            # 提高优先级（需要 root）
  $ renice -n 10 -u username        # 调整某用户所有进程的优先级

  # chrt: 查看和设置实时调度策略（优先级 1-99，值越大优先级越高）
  $ chrt -p 1234                    # 查看进程调度策略
  $ chrt -f 50 ./benchmark          # 以 FIFO 实时调度策略运行，优先级 50
  $ chrt -r 30 ./benchmark          # 以 RR 实时调度策略运行，优先级 30
  $ chrt -o 0 ./benchmark           # 以普通调度策略运行（SCHED_OTHER）
  ```

- 进程绑定处理器

  增加进程的处理器缓存，提高内存 I/O 性能

  ```shell
  # 绑定进程到 CPU 0
  $ taskset -c 0 ./benchmark

  # 绑定进程到 CPU 0, 1, 2
  $ taskset -c 0-2 ./benchmark

  # 查看进程的 CPU 亲和性
  $ taskset -p <PID>
  ```

- 平衡中断

  设置中断在哪个处理器处理

  ```shell
  # 查看系统中断分布情况
  $ cat /proc/interrupts

  # 查看特定中断的 CPU 亲和性（以中断号 44 为例）
  $ cat /proc/irq/44/smp_affinity

  # 设置中断只在 CPU 0 上处理（亲和性掩码 0x1 = CPU 0）
  $ echo 1 > /proc/irq/44/smp_affinity

  # 设置中断在 CPU 0 和 CPU 1 上处理（掩码 0x3 = CPU 0+1）
  $ echo 3 > /proc/irq/44/smp_affinity

  # 设置中断在 CPU 0, 1, 2, 3 上处理（掩码 0xf = CPU 0-3）
  $ echo f > /proc/irq/44/smp_affinity

  # 查看中断亲和性详细信息
  $ cat /proc/irq/44/smp_affinity_list

  # 启动 irqbalance 服务（自动平衡中断到各 CPU）
  $ sudo systemctl start irqbalance

  # 查看 irqbalance 服务状态
  $ sudo systemctl status irqbalance
  ```

- 多核优化

  处理器 0 很重要，具有调度功能

  **CPU 0 的特殊性：**

  CPU 0（也称为 BSP - Bootstrap Processor）在 x86 架构中具有特殊地位：

  1. **启动处理器**：系统启动时，CPU 0 首先执行 BIOS/UEFI 代码，负责初始化其他 CPU（AP - Application Processor）

  2. **中断处理**：Linux 内核默认将某些系统中断（如定时器中断）分配给 CPU 0 处理

  3. **内核任务**：部分内核线程和系统服务默认运行在 CPU 0 上

  4. **调度开销**：CPU 0 承担更多系统级任务，可能导致性能波动

  **优化建议：**

  ```shell
  # 查看 CPU 0 上的中断数量
  $ cat /proc/interrupts | head -1

  # 将性能关键进程绑定到其他 CPU（避开 CPU 0）
  $ taskset -c 1-3 ./benchmark

  # 查看各 CPU 的负载情况
  $ mpstat -P ALL 1

  # 查看 CPU 0 上运行的进程
  $ ps -eo pid,psr,comm | grep -E "^\s*[0-9]+\s+0\s+"

  # 设置进程绑定到 CPU 1, 2, 3（避开 CPU 0）
  $ taskset -pc 1-3 <PID>

  # 查看 NUMA 节点分布
  $ numactl --hardware

  # 在特定 NUMA 节点上运行程序
  $ numactl --cpunodebind=1 --membind=1 ./benchmark
  ```

  **多核优化策略：**

  | 策略           | 说明                                 |
  | -------------- | ------------------------------------ |
  | **避开 CPU 0** | 将关键进程绑定到其他核心             |
  | **负载均衡**   | 使用 `taskset` 分散进程到不同核心    |
  | **NUMA 优化**  | 绑定到同一 NUMA 节点，减少跨节点访问 |
  | **中断隔离**   | 将中断分散到非关键核心               |
  | **CPU 隔离**   | 使用 `isolcpus` 内核参数隔离特定核心 |

  > **NUMA（Non-Uniform Memory Access，非统一内存访问）**

  NUMA 是一种多处理器内存架构，其核心特点是：**不同 CPU 访问不同内存区域的速度不同**。

  **为什么需要 NUMA：**

  传统 SMP（Symmetric Multi-Processing）架构中，所有 CPU 共享同一内存总线：

  ```
  传统 SMP 架构：
  ┌─────────────────────────────────────────┐
  │  CPU0    CPU1    CPU2    CPU3           │
  │    ↘      ↓      ↓      ↙               │
  │      ────────────────────               │
  │            共享内存总线                   │
  │      ────────────────────               │
  │              ↓                          │
  │           主内存                         │
  └─────────────────────────────────────────┘
  问题：CPU 增多时，内存总线成为瓶颈
  ```

  NUMA 架构中，每个 CPU 有本地内存：

  ```
  NUMA 架构：
  ┌─────────────────────────────────────────┐
  │  Node 0: CPU0, CPU1 + 本地内存 64GB      │
  │  Node 1: CPU2, CPU3 + 本地内存 64GB      │
  │                                         │
  │  CPU0 ──本地──┐   CPU2 ──本地──┐         │
  │  CPU1 ──本地──┘   CPU3 ──本地──┘         │
  │       ↕ 互联总线 ↕                       │
  └─────────────────────────────────────────┘
  ```

  **性能差异：**

  | 访问类型       | 延迟       | 说明                   |
  | -------------- | ---------- | ---------------------- |
  | 本地内存访问   | ~100ns     | CPU 访问自己节点的内存 |
  | 跨节点内存访问 | ~150-200ns | CPU 访问其他节点的内存 |

  **NUMA 性能影响：**

  ```shell
  # 查看 NUMA 拓扑
  $ numactl --hardware

  # 查看当前 NUMA 统计
  $ numastat

  # 查看特定进程的 NUMA 内存分布
  $ numastat -p <PID>

  # 绑定进程到 Node 0 的 CPU 和内存
  $ numactl --cpunodebind=0 --membind=0 ./benchmark

  # 绑定到 Node 0 的 CPU，但允许访问所有内存
  $ numactl --cpunodebind=0 ./benchmark

  # 优先分配本地内存，不足时再分配远程内存
  $ numactl --preferred=0 ./benchmark

  # 查看进程的 NUMA 内存策略
  $ numactl --show
  ```

  **NUMA 优化策略：**

  1. **内存本地化**：让进程在本地节点分配内存
  2. **CPU 绑定**：将进程绑定到特定 NUMA 节点
  3. **数据分布**：大数据集按 NUMA 节点分块
  4. **避免跨节点访问**：减少远程内存访问

  **实际应用示例：**

  ```shell
  # 查看系统 NUMA 节点数
  $ lscpu | grep -i numa

  # 查看每个节点的 CPU 列表
  $ lscpu | grep -i "node"

  # 运行内存密集型程序时绑定到本地节点
  $ numactl --cpunodebind=0 --membind=0 ./memory_intensive_app

  # 查看程序运行时的 NUMA 缺页情况
  $ perf stat -e node-loads,node-load-misses ./benchmark
  ```

---

## 内存

**查看内存相关信息**

```shell
# 查看内存概览（总量、已用、空闲、缓存）
$ free -h

# 详细内存信息（包含各类缓存、交换区等）
$ cat /proc/meminfo

# 查看内存硬件信息（型号、频率、插槽）
$ sudo dmidecode -t memory

# 查看每个内存插槽的详细信息（显示 16 行）
$ sudo dmidecode | grep -A16 "Memory Device"

# 查看虚拟内存统计（每秒更新）
$ vmstat 1

# 查看进程内存使用情况
$ top
$ htop

# 查看特定进程的内存映射
$ pmap -x <PID>

# 查看系统内存使用趋势
$ sar -r 1 10

# 查看 NUMA 内存分布
$ numastat

# 查看大页内存信息
$ cat /proc/meminfo | grep -i huge

# 查看内存带宽使用情况（需要 perf）
$ perf stat -e uncore_imc/cas_count_read/,uncore_imc/cas_count_write/ ./benchmark

# 查看内存控制器信息
$ lscpu | grep -i "cache"

# 查看内存错误信息
$ sudo dmidecode -t memory | grep -i error
```

**参数调整**

- 大页配置

  **什么是分页（Paging）：**

  分页是操作系统管理内存的一种机制，将物理内存和虚拟内存划分为固定大小的"页"：

  - **物理页**：物理内存被划分为固定大小的帧（Frame）
  - **虚拟页**：进程的虚拟地址空间被划分为页（Page）
  - **页表**：记录虚拟页到物理页的映射关系

  **为什么需要分页：**

  1. **内存隔离**：每个进程有独立的虚拟地址空间
  2. **内存共享**：多个进程可以共享同一物理页
  3. **按需分配**：只有访问时才分配物理内存
  4. **交换机制**：不常用的页可以交换到磁盘

  **Linux 默认页面大小：**

  - x86/x86_64：4KB
  - ARM：4KB 或 16KB
  - 可通过 `getconf PAGE_SIZE` 查看

  **什么是大页（Huge Pages）：**

  大页使用更大的页面（2MB 或 1GB），减少页表项数量和 TLB Miss：

  | 页面类型 | 大小 | 页表项数量（4GB 内存） | 适用场景       |
  | -------- | ---- | ---------------------- | -------------- |
  | 普通页   | 4KB  | 1,048,576              | 通用场景       |
  | 大页     | 2MB  | 2,048                  | 内存密集型应用 |
  | 巨页     | 1GB  | 4                      | 超大内存应用   |

  **配置命令：**

  ```shell
  # 查看当前页面大小
  $ getconf PAGE_SIZE

  # 查看当前大页配置
  $ grep -i huge /proc/meminfo

  # 设置大页数量（例如 50 个 2MB 大页）
  $ echo 50 | sudo tee /proc/sys/vm/nr_hugepages

  # 永久生效：编辑 /etc/sysctl.conf
  $ echo "vm.nr_hugepages = 50" | sudo tee -a /etc/sysctl.conf
  $ sudo sysctl -p

  # 使用大页运行程序
  $ hugepages --size 2M --nr 50 ./benchmark
  ```

- swap 调整

  **什么是 swap：**

  当物理内存不足时，操作系统将不常用的内存页交换到磁盘的 swap 分区：

  ```
  物理内存不足时：
  ┌─────────────────────────────────────────┐
  │ 物理内存：[活跃页][非活跃页][空闲]           │
  │                      ↓                  │
  │ 将非活跃页交换到磁盘 swap 分区              │
  │                      ↓                  │
  │ 物理内存：[活跃页][空闲]                   │
  └─────────────────────────────────────────┘
  ```

  **内核的交换决策：**

  当内核需要释放内存时，会在两种选择间权衡：
  - 从进程内存中换出一个页
  - 从页缓存（Page Cache）中丢弃一个页

  内核通过以下公式计算交换倾向：

  $swap\_tendency = \frac{mapped\_ratio}{2} + distress + vm\_swappiness$

  - `mapped_ratio`：已映射内存占总内存的比例
  - `distress`：内存压力程度（0-100）
  - `vm_swappiness`：交换倾向参数（0-100）

  **判断逻辑：**
  - $swap\_tendency < 100$：优先从页缓存回收
  - $swap\_tendency \geq 100$：开始交换进程内存

  **查看和调整 swap：**

  ```shell
  # 查看 swap 使用情况
  $ swapon --show
  $ free -h

  # 查看 swap 优先级
  $ cat /proc/swaps

  # 临时调整 swappiness（0-100，值越大越倾向交换）
  $ echo 60 | sudo tee /proc/sys/vm/swappiness

  # 永久调整 swappiness
  $ echo "vm.swappiness = 60" | sudo tee -a /etc/sysctl.conf
  $ sudo sysctl -p

  # 临时禁用 swap
  $ sudo swapoff -a

  # 启用 swap
  $ sudo swapon -a

  # 创建 swap 文件
  $ sudo fallocate -l 4G /swapfile
  $ sudo chmod 600 /swapfile
  $ sudo mkswap /swapfile
  $ sudo swapon /swapfile
  ```

  **性能优化建议：**

  | 场景           | 建议                           |
  | -------------- | ------------------------------ |
  | 数据库服务器   | 设置较低的 swappiness（10-30） |
  | 内存密集型应用 | 减少或禁用 swap                |
  | 通用服务器     | 默认值 60 即可                 |

- 内存同页合并

  **什么是 KSM（Kernel Samepage Merging）：**

  KSM 是 Linux 内核功能，用于合并内容相同的内存页，减少内存使用：

  ```
  KSM 工作原理：
  ┌─────────────────────────────────────────┐
  │ 虚拟机 A: [页1: ABC][页2: XYZ][页3: DEF]  │
  │ 虚拟机 B: [页1: ABC][页2: XYZ][页3: GHI]  │
  │                      ↓                  │
  │ KSM 扫描并合并相同页                       │
  │                      ↓                  │
  │ 物理内存: [ABC][XYZ][DEF][GHI]            │
  │ 页表映射: A→ABC, B→ABC (共享同一物理页)     │
  └─────────────────────────────────────────┘
  ```

  **适用场景：**

  - 运行多个相同操作系统的虚拟机
  - 运行相同应用程序的容器
  - 内存重复率高的环境

  **KSM 相关服务：**

  | 服务       | 作用                        |
  | ---------- | --------------------------- |
  | `ksm`      | 实际扫描内存和合并分页      |
  | `ksmtuned` | 控制 ksm 的扫描策略和积极性 |

  **配置命令：**

  ```shell
  # 启动 ksm 服务
  $ sudo systemctl start ksm
  $ sudo systemctl enable ksm

  # 启动 ksmtuned 服务
  $ sudo systemctl start ksmtuned
  $ sudo systemctl enable ksmtuned

  # 查看 KSM 状态
  $ cat /sys/kernel/mm/ksm/run
  # 0: 停止 1: 运行 2: 单次扫描

  # 查看 KSM 统计信息
  $ cat /sys/kernel/mm/ksm/pages_shared    # 共享的物理页数
  $ cat /sys/kernel/mm/ksm/pages_sharing   # 正在共享的页数
  $ cat /sys/kernel/mm/ksm/pages_unshared  # 未共享的页数
  $ cat /sys/kernel/mm/ksm/pages_volatile  # 易变的页数

  # 计算内存节省量
  # 节省内存 = (pages_sharing - pages_shared) * PAGE_SIZE

  # 配置 ksmtuned（编辑 /etc/ksmtuned.conf）
  $ sudo vi /etc/ksmtuned.conf
  # KSM_NPAGES_MIN=100      # 最小扫描页数
  # KSM_NPAGES_MAX=1000     # 最大扫描页数
  # KSM_SLEEP_MSEC=20       # 扫描间隔（毫秒）

  # 重启服务使配置生效
  $ sudo systemctl restart ksmtuned
  ```

  **注意事项：**

  - KSM 会增加 CPU 开销（扫描和比较内存页）
  - 合并后的页在写入时会产生 Copy-on-Write 开销
  - 适合内存紧张但 CPU 空闲的场景
  - 安全敏感环境需谨慎使用（可能泄露跨虚拟机信息）

- 资源控制

  **ulimit 命令（基础控制）：**

  ulimit 用于限制单个进程的资源使用：

  ```shell
  # 查看当前所有限制
  $ ulimit -a

  # 设置最大虚拟内存（KB）
  $ ulimit -v 1048576  # 限制为 1GB

  # 设置最大内存大小（KB）
  $ ulimit -m 1048576

  # 设置最大文件大小（KB）
  $ ulimit -f 1048576

  # 设置最大进程数
  $ ulimit -u 1024

  # 永久生效：编辑 /etc/security/limits.conf
  $ sudo vi /etc/security/limits.conf
  # username  soft  as  1048576
  # username  hard  as  2097152
  ```

  **cgroup 内存控制（高级控制）：**

  cgroup（Control Groups）提供更精细的资源控制：

  | 参数                          | 说明                               |
  | ----------------------------- | ---------------------------------- |
  | `memory.limit_in_bytes`       | 最大用户内存（包括文件缓存）       |
  | `memory.memsw.limit_in_bytes` | 最大内存 + 交换空间                |
  | `memory.swappiness`           | 交换倾向（类似 vm.swappiness）     |
  | `memory.oom_control`          | OOM 终结器控制（0: 启用, 1: 禁用） |

  **cgroup v1 配置示例：**

  ```shell
  # 创建 cgroup
  $ sudo mkdir /sys/fs/cgroup/memory/mygroup

  # 设置内存限制（例如 512MB）
  $ echo 536870912 | sudo tee /sys/fs/cgroup/memory/mygroup/memory.limit_in_bytes

  # 设置内存 + 交换空间限制（例如 1GB）
  $ echo 1073741824 | sudo tee /sys/fs/cgroup/memory/mygroup/memory.memsw.limit_in_bytes

  # 将进程加入 cgroup
  $ echo <PID> | sudo tee /sys/fs/cgroup/memory/mygroup/cgroup.procs

  # 禁用 OOM 终结器
  $ echo 1 | sudo tee /sys/fs/cgroup/memory/mygroup/memory.oom_control

  # 查看 cgroup 内存使用
  $ cat /sys/fs/cgroup/memory/mygroup/memory.usage_in_bytes
  ```

  **cgroup v2 配置示例（推荐）：**

  ```shell
  # 挂载 cgroup v2
  $ sudo mount -t cgroup2 none /sys/fs/cgroup

  # 创建子组
  $ sudo mkdir /sys/fs/cgroup/mygroup

  # 设置内存限制
  $ echo "max 536870912" | sudo tee /sys/fs/cgroup/mygroup/memory.max

  # 设置内存 + 交换限制
  $ echo "1073741824" | sudo tee /sys/fs/cgroup/mygroup/memory.swap.max

  # 将进程加入子组
  $ echo <PID> | sudo tee /sys/fs/cgroup/mygroup/cgroup.procs

  # 查看内存使用
  $ cat /sys/fs/cgroup/mygroup/memory.current
  ```

  **systemd 集成（推荐用于服务）：**

  ```shell
  # 编辑服务文件
  $ sudo systemctl edit myservice

  # 添加以下内容：
  [Service]
  MemoryLimit=512M
  MemoryMax=512M
  MemorySwapMax=1G

  # 重启服务
  $ sudo systemctl restart myservice
  ```

  **实际应用场景：**

  | 场景     | 控制方式     | 说明     |
  | -------- | ------------ | -------- |
  | 单个进程 | ulimit       | 简单快速 |
  | 服务管理 | systemd      | 集成度高 |
  | 容器环境 | cgroup       | 精细控制 |
  | 虚拟机   | cgroup + KSM | 内存优化 |

---

## 文件系统

文件系统是操作系统与磁盘设备之间的桥梁，负责数据存储、管理和完整性保证。

**配置检查：**

```shell
# 查看已挂载的文件系统
$ mount | grep -E "ext[234]|xfs|btrfs"

# 查看文件系统类型和使用情况
$ df -Th

# 查看文件系统详细信息（以 /dev/sda1 为例）
$ sudo tune2fs -l /dev/sda1

# 查看文件系统是否启用访问时间戳
$ sudo tune2fs -l /dev/sda1 | grep "Default mount options"

# 查看文件系统缓存使用情况
$ cat /proc/meminfo | grep -i cache

# 使用 vmstat 监控文件系统 I/O
$ vmstat 1

# 使用 sar 监控磁盘 I/O
$ sar -d 1 10

# 使用 SystemTap 跟踪文件系统事件（需要安装 systemtap）
$ sudo stap -e 'probe vfs.read { printf("%s %d\n", execname(), pid()) }'
```

**参数调整：**

1. **文件描述符调优：**

   文件描述符是系统分配给应用程序的 I/O 句柄，限制了同时打开的文件/连接数。

   ```shell
   # 查看当前用户级文件描述符限制
   $ ulimit -n

   # 临时设置文件描述符限制（当前会话有效）
   $ ulimit -SHn 65535

   # 永久设置：编辑 /etc/security/limits.conf
   $ sudo vi /etc/security/limits.conf
   # 添加以下两行：
   # username  soft  nofile  65535
   # username  hard  nofile  65535

   # 查看系统级文件描述符限制
   $ cat /proc/sys/fs/file-max

   # 临时修改系统级限制
   $ echo 2097152 | sudo tee /proc/sys/fs/file-max

   # 永久修改：编辑 /etc/sysctl.conf
   $ echo "fs.file-max = 2097152" | sudo tee -a /etc/sysctl.conf
   $ sudo sysctl -p
   ```

2. **内核参数调优：**

   ```shell
   # 查看所有内核参数
   $ sudo sysctl -a

   # 查看文件系统相关参数
   $ sudo sysctl -a | grep fs

   # 设置 epoll 最大监听事件数
   $ echo 1048576 | sudo tee /proc/sys/fs/epoll/max_user_watches

   # 设置 inotify 最大监控数
   $ echo 524288 | sudo tee /proc/sys/fs/inotify/max_user_watches

   # 设置 aio 最大并发数
   $ echo 1048576 | sudo tee /proc/sys/fs/aio-max-nr
   ```

3. **文件系统挂载选项：**

   ```shell
   # 查看当前挂载选项
   $ mount | grep " / "

   # 常用优化挂载选项（编辑 /etc/fstab）
   # /dev/sda1  /  ext4  defaults,noatime,nodiratime,barrier=0  0  1

   # 选项说明：
   # noatime      - 不更新文件访问时间（减少写操作）
   # nodiratime   - 不更新目录访问时间
   # barrier=0    - 禁用写屏障（提高性能，但降低数据安全性）
   # data=writeback - 日志模式（提高性能，但可能丢失数据）
   ```

4. **文件系统调整工具：**

   ```shell
   # 调整 ext4 文件系统参数
   $ sudo tune2fs -c 30 /dev/sda1        # 每 30 次挂载后检查
   $ sudo tune2fs -i 0 /dev/sda1         # 禁用基于时间的检查
   $ sudo tune2fs -m 1 /dev/sda1         # 保留块比例设为 1%

   # 调整 XFS 文件系统参数
   $ sudo xfs_info /dev/sda1
   $ sudo xfs_admin -l /dev/sda1         # 查看日志信息
   ```

**性能监控工具：**

| 工具        | 用途         |
| ----------- | ------------ |
| `vmstat`    | 虚拟内存统计 |
| `sar`       | 系统活动报告 |
| `iostat`    | I/O 统计     |
| `iotop`     | I/O 监控     |
| `blktrace`  | 块设备追踪   |
| `SystemTap` | 动态追踪     |

---

## 磁盘

磁盘是速度较慢的存储子系统，通常会成为系统性能瓶颈。当高负载下磁盘成为瓶颈时，CPU 会空闲等待 I/O 完成。

**配置检查：**

```shell
# 查看系统硬盘信息
$ sudo fdisk -l

# 查看磁盘使用情况
$ df -Th

# 查看分区信息
$ cat /proc/partitions

# 评估磁盘性能（iostat）
$ iostat -x 1 10

# 使用 sar 监控磁盘 I/O
$ sar -d 1 10

# 使用 top 查看是否受 I/O 限制（查看 %wa 指标）
$ top

# 查看磁盘 I/O 统计
$ cat /proc/diskstats

# 查看磁盘队列深度
$ cat /sys/block/sda/queue/nr_requests

# 使用 iotop 监控进程 I/O
$ sudo iotop -o
```

**参数调整：**

1. **进程 I/O 优先级（ionice）：**

   ionice 将磁盘 I/O 调度分为三类：

   | 类别                | 说明                         | 适用场景 |
   | ------------------- | ---------------------------- | -------- |
   | 实时（Realtime）    | 最高优先级，可能饿死其他进程 | 关键任务 |
   | 尽力（Best-effort） | 默认调度，公平分配           | 通用场景 |
   | 空闲（Idle）        | 仅在磁盘空闲时运行           | 备份任务 |

   ```shell
   # 设置进程为空闲 I/O 优先级（适合备份任务）
   $ sudo ionice -c 3 -p 1623

   # 设置进程为实时 I/O 优先级，级别 1
   $ sudo ionice -c 1 -n 1 -p 1623

   # 查看进程 I/O 优先级
   $ ionice -p 1623

   # 以空闲优先级运行命令
   $ sudo ionice -c 3 ./backup_script.sh
   ```

2. **I/O 调度器选择：**

   Linux 内核提供三种 I/O 调度器：

   | 调度器       | 特点                                 | 适用场景         |
   | ------------ | ------------------------------------ | ---------------- |
   | **CFQ**      | 完全公平调度器，为每个进程分配时间片 | 通用场景（默认） |
   | **noop**     | 先进先出，最简单                     | 虚拟机、SSD      |
   | **deadline** | 保证请求截止时间                     | 数据库、实时系统 |

   ```shell
   # 查看当前 I/O 调度器
   $ cat /sys/block/sda/queue/scheduler

   # 临时修改 I/O 调度器
   $ echo deadline | sudo tee /sys/block/sda/queue/scheduler

   # 永久修改：编辑 /etc/default/grub
   $ sudo vi /etc/default/grub
   # GRUB_CMDLINE_LINUX="elevator=deadline"
   $ sudo update-grub

   # 查看调度器详细参数
   $ ls /sys/block/sda/queue/iosched/
   ```

3. **磁盘队列调优：**

   ```shell
   # 查看队列深度
   $ cat /sys/block/sda/queue/nr_requests

   # 调整队列深度（适合 SSD）
   $ echo 256 | sudo tee /sys/block/sda/queue/nr_requests

   # 查看预读设置
   $ cat /sys/block/sda/queue/read_ahead_kb

   # 调整预读大小（适合顺序读取）
   $ echo 2048 | sudo tee /sys/block/sda/queue/read_ahead_kb

   # 禁用磁盘缓存（适合数据库）
   $ sudo hdparm -W0 /dev/sda
   ```

4. **磁盘性能优化建议：**

   | 场景     | 优化策略                       |
   | -------- | ------------------------------ |
   | 数据库   | deadline 调度器 + 禁用磁盘缓存 |
   | 虚拟机   | noop 调度器                    |
   | SSD      | noop 调度器 + 队列深度 256     |
   | 顺序读写 | 增大预读值                     |
   | 备份任务 | ionice -c 3（空闲优先级）      |

**I/O 问题诊断流程：**

```
1. 使用 top 查看 %wa（I/O 等待）
   ↓
2. 使用 iostat -x 查看磁盘利用率
   ↓
3. 使用 iotop 找出高 I/O 进程
   ↓
4. 使用 strace 分析进程系统调用
   ↓
5. 根据场景选择优化策略
```

---

## 网络

随着计算节点规模扩大，网络性能对整体系统影响越来越大。

**配置检查：**

```shell
# 查看网络接口信息
$ ifconfig
$ ip addr show

# 查看网关地址
$ netstat -rn

# 查看路由表
$ netstat -r

# 查看网络接口状态（RX-ERR/TX-ERR 应为 0）
$ netstat -i

# 查看网络连接状态
$ netstat -an | grep ESTABLISHED | wc -l

# 查看网络流量
$ sar -n DEV 1 10

# 查看网络错误统计
$ sar -n EDEV 1 10

# 查看 TCP 连接状态
$ ss -s

# 使用 iperf 测试网络带宽
$ iperf -s  # 服务端
$ iperf -c <server_ip>  # 客户端
```

**参数调整：**

1. **套接字缓冲区调优：**

   ```shell
   # 查看 TCP 读缓冲区（最小值 默认值 最大值）
   $ cat /proc/sys/net/ipv4/tcp_rmem
   # 4096  87380  6291456

   # 查看 TCP 写缓冲区
   $ cat /proc/sys/net/ipv4/tcp_wmem
   # 4096  16384  4194304

   # 设置 TCP 读缓冲区（最小 4KB，默认 87KB，最大 6MB）
   $ echo "4096 87380 6291456" | sudo tee /proc/sys/net/ipv4/tcp_rmem

   # 设置 TCP 写缓冲区
   $ echo "4096 16384 4194304" | sudo tee /proc/sys/net/ipv4/tcp_wmem

   # 设置 UDP 缓冲区
   $ echo 212992 | sudo tee /proc/sys/net/udp_mem_min

   # 设置套接字最大缓冲区
   $ echo 212992 | sudo tee /proc/sys/net/core/wmem_max
   $ echo 212992 | sudo tee /proc/sys/net/core/rmem_max
   ```

2. **TCP 积压队列调优：**

   ```shell
   # 查看 TCP 积压队列大小
   $ cat /proc/sys/net/ipv4/tcp_max_syn_backlog
   # 默认 1024

   # 增大 TCP 积压队列（适合高并发服务器）
   $ echo 65535 | sudo tee /proc/sys/net/ipv4/tcp_max_syn_backlog

   # 查看监听队列最大长度
   $ cat /proc/sys/net/core/somaxconn
   # 默认 128

   # 增大监听队列
   $ echo 65535 | sudo tee /proc/sys/net/core/somaxconn

   # 查看 SYN cookies（防止 SYN 洪水攻击）
   $ cat /proc/sys/net/ipv4/tcp_syncookies
   # 1: 启用  0: 禁用
   ```

3. **设备积压队列调优：**

   ```shell
   # 查看网络设备积压队列
   $ cat /proc/sys/net/core/netdev_max_backlog
   # 默认 1000

   # 增大积压队列（适合高速网络）
   $ echo 65535 | sudo tee /proc/sys/net/core/netdev_max_backlog

   # 查看网络设备队列长度
   $ ip link show eth0 | grep qlen
   # 默认 1000

   # 增大队列长度
   $ sudo ip link set eth0 txqueuelen 10000
   ```

4. **TCP 拥塞控制调优：**

   ```shell
   # 查看可用的拥塞控制算法
   $ cat /proc/sys/net/ipv4/tcp_available_congestion_control
   # cubic reno bbr

   # 查看当前拥塞控制算法
   $ cat /proc/sys/net/ipv4/tcp_congestion_control
   # 默认 cubic

   # 切换到 BBR（适合高带宽高延迟网络）
   $ echo bbr | sudo tee /proc/sys/net/ipv4/tcp_congestion_control

   # 启用 BBR 模块
   $ sudo modprobe tcp_bbr
   $ echo "tcp_bbr" | sudo tee -a /etc/modules-load.d/bbr.conf
   ```

5. **TCP 选项调优：**

   ```shell
   # 启用 TCP 时间戳
   $ echo 1 | sudo tee /proc/sys/net/ipv4/tcp_timestamps

   # 启用 TCP 窗口缩放
   $ echo 1 | sudo tee /proc/sys/net/ipv4/tcp_window_scaling

   # 启用 TCP 选择性确认
   $ echo 1 | sudo tee /proc/sys/net/ipv4/tcp_sack

   # 设置 TCP 最大段大小（MSS）
   $ echo 1460 | sudo tee /proc/sys/net/ipv4/tcp_mss

   # 启用 TCP 快速回收
   $ echo 1 | sudo tee /proc/sys/net/ipv4/tcp_tw_reuse

   # 设置 TIME_WAIT 状态最大数量
   $ echo 65535 | sudo tee /proc/sys/net/ipv4/tcp_max_tw_buckets
   ```

6. **网络接口调优：**

   ```shell
   # 查看网卡队列数
   $ ethtool -l eth0

   # 设置网卡多队列
   $ sudo ethtool -L eth0 combined 8

   # 查看网卡 Ring Buffer 大小
   $ ethtool -g eth0

   # 增大 Ring Buffer
   $ sudo ethtool -G eth0 rx 4096 tx 4096

   # 启用网卡 TSO（TCP Segmentation Offload）
   $ sudo ethtool -K eth0 tso on

   # 启用网卡 GRO（Generic Receive Offload）
   $ sudo ethtool -K eth0 gro on

   # 查看网卡中断亲和性
   $ cat /proc/interrupts | grep eth0
   ```

**永久生效配置：**

```shell
# 编辑 /etc/sysctl.conf
$ sudo vi /etc/sysctl.conf

# 添加以下配置
net.ipv4.tcp_rmem = 4096 87380 6291456
net.ipv4.tcp_wmem = 4096 16384 4194304
net.ipv4.tcp_max_syn_backlog = 65535
net.core.somaxconn = 65535
net.core.netdev_max_backlog = 65535
net.ipv4.tcp_congestion_control = bbr
net.ipv4.tcp_timestamps = 1
net.ipv4.tcp_window_scaling = 1
net.ipv4.tcp_sack = 1
net.ipv4.tcp_tw_reuse = 1

# 使配置生效
$ sudo sysctl -p
```

**网络性能优化建议：**

| 场景              | 优化策略                             |
| ----------------- | ------------------------------------ |
| 高并发 Web 服务器 | 增大 somaxconn + tcp_max_syn_backlog |
| 高带宽传输        | 启用 BBR + 增大缓冲区                |
| 低延迟应用        | 启用 TCP 时间戳 + 窗口缩放           |
| 大量短连接        | 启用 tcp_tw_reuse                    |
| 万兆网络          | 多队列 + Ring Buffer + GRO/TSO       |
