```c++
for (int i = 0; i < N; i++) {
    for (int j = 0; j < N; j++) {
        for (int k = 0; k < N; k++) {
            result[i][j] += a[i][k] * b[k][j];
        }
    }
}
```



```shell
$ cmake --build . --target benchmarkLab
[100%] Built target lab
2026-05-19T19:30:05+08:00
Running ./lab
Run on (12 X 4100 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x6)
  L1 Instruction 32 KiB (x6)
  L2 Unified 256 KiB (x6)
  L3 Unified 9216 KiB (x1)
Load Average: 1.25, 1.42, 1.85
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
---------------------------------------------------------------
Benchmark                     Time             CPU   Iterations
---------------------------------------------------------------
bench1/iterations:10 1177240010 ns   1175749439 ns           10
[100%] Built target benchmarkLab
```

```shell
$ perf stat --topdown -a taskset -c 0 ./lab
2026-05-19T19:34:07+08:00
Running ./lab
Run on (12 X 4100 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x6)
  L1 Instruction 32 KiB (x6)
  L2 Unified 256 KiB (x6)
  L3 Unified 9216 KiB (x1)
Load Average: 2.25, 1.79, 1.89
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
---------------------------------------------------------------
Benchmark                     Time             CPU   Iterations
---------------------------------------------------------------
bench1/iterations:10 1337887910 ns   1290322736 ns           10

 Performance counter stats for 'system wide':

 %  tma_bad_speculation %  tma_backend_bound      %  tma_retiring %  tma_frontend_bound 
                    3.8                 40.0                    31.1                   25.1 

      13.387162961 seconds time elapsed
```

```shell
$ perf stat --topdown --per-core -a ./lab
2026-05-19T19:34:58+08:00
Running ./lab
Run on (12 X 4100 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x6)
  L1 Instruction 32 KiB (x6)
  L2 Unified 256 KiB (x6)
  L3 Unified 9216 KiB (x1)
Load Average: 1.84, 1.75, 1.87
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
---------------------------------------------------------------
Benchmark                     Time             CPU   Iterations
---------------------------------------------------------------
bench1/iterations:10 1187155625 ns   1187060212 ns           10

 Performance counter stats for 'system wide':

                   %  tma_bad_speculation %  tma_backend_bound      %  tma_retiring %  tma_frontend_bound 
S0-D0-C0              2                     6.4                 41.3                    11.8                   40.5 
S0-D0-C1              2                     0.8                 51.1                    38.6                    9.5 
S0-D0-C2              2                     5.1                 33.2                    30.6                   31.1 
S0-D0-C3              2                     6.5                 31.6                    24.5                   37.3 
S0-D0-C4              2                     8.6                 31.0                    17.1                   43.3 
S0-D0-C5              2                     8.6                 29.3                    16.4                   45.8 

      11.878848956 seconds time elapsed

```



```shell
$ toplev.py --core S0-C0 -l2 --no-desc taskset -c 0 ./lab
Consider disabling nmi watchdog to minimize multiplexing
(echo 0 | sudo tee /proc/sys/kernel/nmi_watchdog or
 echo kernel.nmi_watchdog=0 >> /etc/sysctl.conf ; sysctl -p as root)
Downloading https://raw.githubusercontent.com/intel/perfmon/main/mapfile.csv to mapfile.csv
Downloading https://raw.githubusercontent.com/intel/perfmon/main/SKL/events/skylake_core.json to GenuineIntel-6-9E-core.json
Downloading https://raw.githubusercontent.com/intel/perfmon/main/README.md to README.md
Downloading https://raw.githubusercontent.com/intel/perfmon/main/LICENSE to LICENSE
Downloading https://raw.githubusercontent.com/intel/perfmon/main/SKL/events/skylake_uncore.json to GenuineIntel-6-9E-uncore.json
Will measure complete system.
2026-05-19T19:41:47+08:00
Running ./lab
Run on (12 X 4100 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x6)
  L1 Instruction 32 KiB (x6)
  L2 Unified 256 KiB (x6)
  L3 Unified 9216 KiB (x1)
Load Average: 1.48, 1.66, 1.79
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
---------------------------------------------------------------
Benchmark                     Time             CPU   Iterations
---------------------------------------------------------------
bench1/iterations:10 1350692536 ns   1308104473 ns           10
# 5.01-full-perf on Intel(R) Core(TM) i7-8750H CPU @ 2.20GHz [cfl/skylake]
C0    BE               Backend_Bound               % Slots                       51.0   [ 8.0%]
C0    BE/Mem           Backend_Bound.Memory_Bound  % Slots                       21.5   [ 8.0%]
C0    BE/Core          Backend_Bound.Core_Bound    % Slots                       29.5   [ 8.0%]<==
C0-T0 MUX                                          %                              8.00 
C0-T1 MUX                                          %                              8.00 
Run toplev --describe Core_Bound^ to get more information on bottleneck
Add --run-sample to find locations
Add --nodes '!+Core_Bound*/3,+MUX' for breakdown.
```





to use advisor see the source code

```shell
cmake -E make_directory build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --config Release --parallel 8
cmake --build . --target validateLab
cmake --build . --target benchmarkLab

|

cmake -E make_directory build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_FLAGS="-g" -DCMAKE_CXX_FLAGS="-g" ..
cmake --build . --config Release --parallel 8
cmake --build . --target validateLab
cmake --build . --target benchmarkLab
```



---



```c++
for (int i = 0; i < N; i++) {
    for (int k = 0; k < N; k++) {
        for (int j = 0; j < N; j++) {
            result[i][j] += a[i][k] * b[k][j];
        }
    }
}
```

```shell
$ cmake --build . --target benchmarkLab
[100%] Built target lab
2026-05-19T21:05:30+08:00
Running ./lab
Run on (12 X 4100 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x6)
  L1 Instruction 32 KiB (x6)
  L2 Unified 256 KiB (x6)
  L3 Unified 9216 KiB (x1)
Load Average: 1.81, 1.75, 2.22
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
---------------------------------------------------------------
Benchmark                     Time             CPU   Iterations
---------------------------------------------------------------
bench1/iterations:10  139345511 ns    139269044 ns           10
[100%] Built target benchmarkLab
```



```shell
$ perf stat --topdown --per-core -a ./lab
2026-05-19T21:06:49+08:00
Running ./lab
Run on (12 X 4100 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x6)
  L1 Instruction 32 KiB (x6)
  L2 Unified 256 KiB (x6)
  L3 Unified 9216 KiB (x1)
Load Average: 1.57, 1.70, 2.16
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
---------------------------------------------------------------
Benchmark                     Time             CPU   Iterations
---------------------------------------------------------------
bench1/iterations:10  114180944 ns    114178225 ns           10

 Performance counter stats for 'system wide':

                   %  tma_bad_speculation %  tma_backend_bound      %  tma_retiring %  tma_frontend_bound 
S0-D0-C0              2                     7.1                 37.0                    12.5                   43.3 
S0-D0-C1              2                     0.2                 43.6                    54.7                    1.4 
S0-D0-C2              2                     7.7                 24.0                    21.9                   46.3 
S0-D0-C3              2                     7.8                 25.9                    14.5                   51.7 
S0-D0-C4              2                     5.0                 36.5                    32.3                   26.3 
S0-D0-C5              2                     6.5                 23.9                    32.7                   36.9 

       1.149557780 seconds time elapsed
```



```shell
$ toplev.py --core S0-C0 -l2 --no-desc taskset -c 0 ./lab
Consider disabling nmi watchdog to minimize multiplexing
(echo 0 | sudo tee /proc/sys/kernel/nmi_watchdog or
 echo kernel.nmi_watchdog=0 >> /etc/sysctl.conf ; sysctl -p as root)
Will measure complete system.
2026-05-19T21:08:07+08:00
Running ./lab
Run on (12 X 4100 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x6)
  L1 Instruction 32 KiB (x6)
  L2 Unified 256 KiB (x6)
  L3 Unified 9216 KiB (x1)
Load Average: 1.66, 1.68, 2.12
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
---------------------------------------------------------------
Benchmark                     Time             CPU   Iterations
---------------------------------------------------------------
bench1/iterations:10  128479473 ns    127607183 ns           10
# 5.01-full-perf on Intel(R) Core(TM) i7-8750H CPU @ 2.20GHz [cfl/skylake]
C0    BE               Backend_Bound               % Slots                       48.0   [ 8.0%]
C0    BE/Mem           Backend_Bound.Memory_Bound  % Slots                       20.0   [ 8.0%]
C0    BE/Core          Backend_Bound.Core_Bound    % Slots                       28.0   [ 8.0%]<==
C0-T0 MUX                                          %                              8.00 
C0-T1 MUX                                          %                              8.00 
Run toplev --describe Core_Bound^ to get more information on bottleneck
Add --run-sample to find locations
Add --nodes '!+Core_Bound*/3,+MUX' for breakdown.
```

