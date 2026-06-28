# TMA

## 环境搭建

### google benchmark 工具下载

```bash
$ cd TMA
$ ./tools/make_benchmark_library.sh
```

### 环境测试

```bash
cmake -E make_directory build

cd build

cmake -DCMAKE_BUILD_TYPE=Release ..
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_FLAGS="-g" -DCMAKE_CXX_FLAGS="-g" ..

cmake --build . --config Release --parallel 8

cmake --build . --target validateLab

cmake --build . --target benchmarkLab

```

### Performance Profile

执行 `cmake --build . --target benchmarkLab` 时会出现如下结果
默认执行会出现如下 `WARNING`
`***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will `
```bash
hmarkLab
[100%] Built target lab
2026-06-13T21:57:03+08:00
Running ./lab
Run on (24 X 5400 MHz CPU s)
CPU Caches:
  L1 Data 48 KiB (x24)
  L1 Instruction 64 KiB (x24)
  L2 Unified 3072 KiB (x24)
  L3 Unified 36864 KiB (x1)
Load Average: 0.41, 0.27, 0.41
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
-----------------------------------------------------
Benchmark           Time             CPU   Iterations
-----------------------------------------------------
bench1           23.0 ns         23.0 ns    118609314
[100%] Built target benchmarkLab
```

设置 CPU 为性能模式消除
Set the frequency scaling governor to `performance`.
```
sudo cpupower frequency-set --governor performance
```



```bash
perf record ./lab
perf report
```

执行 perf 会报权限问题

```bash
$ perf stat --topdown -a taskset -c 0 ./lab
Error:
No supported events found.
Access to performance monitoring and observability operations is limited.
Consider adjusting /proc/sys/kernel/perf_event_paranoid setting to open
access to performance monitoring and observability operations for processes
without CAP_PERFMON, CAP_SYS_PTRACE or CAP_SYS_ADMIN Linux capability.
More information can be found at 'Perf events and tool security' document:
https://www.kernel.org/doc/html/latest/admin-guide/perf-security.html
perf_event_paranoid setting is 4:
  -1: Allow use of (almost) all events by all users
      Ignore mlock limit after perf_event_mlock_kb without CAP_IPC_LOCK
>= 0: Disallow raw and ftrace function tracepoint access
>= 1: Disallow CPU event access
>= 2: Disallow kernel profiling
To make the adjusted perf_event_paranoid setting permanent preserve it
in /etc/sysctl.conf (e.g. kernel.perf_event_paranoid = <setting>)
```

执行以下命令解决

```bash
$ sudo sysctl -w kernel.perf_event_paranoid=-1
kernel.perf_event_paranoid = -1
```

do some optimize

rebuild



### 其他说明


## TMA

1. Memory Bound
   1. Data Packing
   2. Loop Interchange 1
   3. Loop Interchange 2
   4. SW Memory Prefetch
   5. Loop Tiling 1
   6. False Sharing
2. Core Bound
   1. Vectorization 1
   2. Vectorization 2
   3. Function Inlining 1
   4. Compiler Intrinsics 1
   5. Compiler Intrinsics 2
   6. Dependency Chains 1
3. Bad Speculation
   1. Replacing Branches with Lookup Tables
4. misc
   1. Link Time Optimization
   2. Profile Guided Optimizations


