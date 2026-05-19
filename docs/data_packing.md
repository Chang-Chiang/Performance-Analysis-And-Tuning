
---
```c++
struct S {
    int i;
    long long l;
    short s;
    double d;
    bool b;

    bool operator<(const S &s) const { return this->i < s.i; }
};

// check sizeof S during compiling
template <int N>
class TD;
// never compiles but shows the value of sizeof(s)
TD<sizeof(S)> td;
```

```shell
$ cmake --build . --config Release --parallel 8
[ 37%] Building CXX object CMakeFiles/lab.dir/bench.cpp.o
[ 37%] Building CXX object CMakeFiles/lab.dir/init.cpp.o
[ 37%] Building CXX object CMakeFiles/lab.dir/solution.cpp.o
[ 50%] Building CXX object CMakeFiles/validate.dir/init.cpp.o
[ 62%] Building CXX object CMakeFiles/validate.dir/solution.cpp.o
[ 75%] Building CXX object CMakeFiles/validate.dir/validate.cpp.o
In file included from /home/cc/Projects/Performance-Analysis-And-Tuning/memory_bound/data_packing/init.cpp:1:
/home/cc/Projects/Performance-Analysis-And-Tuning/memory_bound/data_packing/solution.h:33:15: error: aggregate ‘TD<40> td’ has incomplete type and cannot be defined
   33 | TD<sizeof(S)> td;
```

```
40 bytes
```

```shell
$ cmake --build . --target benchmarkLab
[100%] Built target lab
2026-05-19T08:05:24+08:00
Running ./lab
Run on (12 X 4100 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x6)
  L1 Instruction 32 KiB (x6)
  L2 Unified 256 KiB (x6)
  L3 Unified 9216 KiB (x1)
Load Average: 1.44, 1.70, 2.02
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
-----------------------------------------------------
Benchmark           Time             CPU   Iterations
-----------------------------------------------------
bench1           57.0 ms         57.0 ms           50
[100%] Built target benchmarkLab
```

```shell
$ perf stat --topdown -a taskset -c 0 ./lab
2026-05-19T08:06:30+08:00
Running ./lab
Run on (12 X 4100 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x6)
  L1 Instruction 32 KiB (x6)
  L2 Unified 256 KiB (x6)
  L3 Unified 9216 KiB (x1)
Load Average: 1.82, 1.74, 2.01
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
-----------------------------------------------------
Benchmark           Time             CPU   Iterations
-----------------------------------------------------
bench1           57.7 ms         57.2 ms           12

 Performance counter stats for 'system wide':

 %  tma_bad_speculation %  tma_backend_bound      %  tma_retiring %  tma_frontend_bound 
                    3.8                 48.1                    24.4                   23.7 

       0.818694856 seconds time elapsed
```

```shell
$ perf stat --topdown --per-core -a ./lab
2026-05-19T08:07:02+08:00
Running ./lab
Run on (12 X 4100 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x6)
  L1 Instruction 32 KiB (x6)
  L2 Unified 256 KiB (x6)
  L3 Unified 9216 KiB (x1)
Load Average: 1.88, 1.77, 2.01
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
-----------------------------------------------------
Benchmark           Time             CPU   Iterations
-----------------------------------------------------
bench1           55.8 ms         55.8 ms           12

 Performance counter stats for 'system wide':

                   %  tma_bad_speculation %  tma_backend_bound      %  tma_retiring %  tma_frontend_bound 
S0-D0-C0              2                     6.1                 41.1                    10.5                   42.3 
S0-D0-C1              2                     6.5                 29.0                    17.6                   46.9 
S0-D0-C2              2                     1.7                 64.5                    22.7                   11.1 
S0-D0-C3              2                     5.6                 31.3                    10.2                   52.9 
S0-D0-C4              2                     6.9                 22.5                    17.3                   53.2 
S0-D0-C5              2                     5.3                 35.7                    28.2                   30.8 

       0.797945886 seconds time elapsed
```





---

```c++
struct S {
    float          d;
    long long      l : 16;
    int            i : 8;
    unsigned short s : 7;
    bool           b : 1;

    bool operator<(const S &s) const { return this->i < s.i; }
};

// check sizeof S during compiling
template <int N>
class TD;
// never compiles but shows the value of sizeof(s)
TD<sizeof(S)> td;
```

```shell
$ cmake --build . --config Release --parallel 8
[ 25%] Building CXX object CMakeFiles/validate.dir/validate.cpp.o
[ 25%] Building CXX object CMakeFiles/lab.dir/bench.cpp.o
[ 37%] Building CXX object CMakeFiles/lab.dir/solution.cpp.o
[ 50%] Building CXX object CMakeFiles/validate.dir/init.cpp.o
[ 62%] Building CXX object CMakeFiles/validate.dir/solution.cpp.o
[ 75%] Building CXX object CMakeFiles/lab.dir/init.cpp.o
In file included from /home/cc/Projects/Performance-Analysis-And-Tuning/memory_bound/data_packing/validate.cpp:1:
/home/cc/Projects/Performance-Analysis-And-Tuning/memory_bound/data_packing/solution.h:33:15: error: aggregate ‘TD<8> td’ has incomplete type and cannot be defined
   33 | TD<sizeof(S)> td;
      |               ^~
```

```
8 bytes
```

```shell
$ cmake --build . --target benchmarkLab
[ 25%] Building CXX object CMakeFiles/lab.dir/bench.cpp.o
[ 50%] Building CXX object CMakeFiles/lab.dir/init.cpp.o
[ 75%] Building CXX object CMakeFiles/lab.dir/solution.cpp.o
[100%] Linking CXX executable lab
[100%] Built target lab
2026-05-19T08:11:14+08:00
Running ./lab
Run on (12 X 4100 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x6)
  L1 Instruction 32 KiB (x6)
  L2 Unified 256 KiB (x6)
  L3 Unified 9216 KiB (x1)
Load Average: 1.60, 1.68, 1.91
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
-----------------------------------------------------
Benchmark           Time             CPU   Iterations
-----------------------------------------------------
bench1           9.20 ms         9.18 ms          304
[100%] Built target benchmarkLab
```

```shell
$ perf stat --topdown -a taskset -c 0 ./lab
2026-05-19T08:11:53+08:00
Running ./lab
Run on (12 X 4100 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x6)
  L1 Instruction 32 KiB (x6)
  L2 Unified 256 KiB (x6)
  L3 Unified 9216 KiB (x1)
Load Average: 1.57, 1.67, 1.90
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
-----------------------------------------------------
Benchmark           Time             CPU   Iterations
-----------------------------------------------------
bench1           9.79 ms         9.72 ms           64

 Performance counter stats for 'system wide':

 %  tma_bad_speculation %  tma_backend_bound      %  tma_retiring %  tma_frontend_bound 
                    3.3                 41.2                    34.2                   21.2 

       0.810288951 seconds time elapsed
```

```shell
$ perf stat --topdown --per-core -a ./lab
2026-05-19T08:12:39+08:00
Running ./lab
Run on (12 X 4100 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x6)
  L1 Instruction 32 KiB (x6)
  L2 Unified 256 KiB (x6)
  L3 Unified 9216 KiB (x1)
Load Average: 1.80, 1.73, 1.91
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
-----------------------------------------------------
Benchmark           Time             CPU   Iterations
-----------------------------------------------------
bench1           9.35 ms         9.35 ms           68

 Performance counter stats for 'system wide':

                   %  tma_bad_speculation %  tma_backend_bound      %  tma_retiring %  tma_frontend_bound 
S0-D0-C0              2                     5.2                 35.4                    27.6                   31.7 
S0-D0-C1              2                     0.7                 50.0                    44.3                    5.1 
S0-D0-C2              2                     6.6                 21.8                    26.8                   44.8 
S0-D0-C3              2                     9.3                 27.7                    13.0                   50.0 
S0-D0-C4              2                     6.9                 34.7                    10.3                   48.1 
S0-D0-C5              2                     6.6                 33.6                    10.1                   49.7 

       0.803051321 seconds time elapsed
```



---











