```shell
cmake -E make_directory build
cd build
# cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_FLAGS="-g" -DCMAKE_CXX_FLAGS="-g" ..
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --config Release --parallel 8
cmake --build . --target validateLab
cmake --build . --target benchmarkLab
```



```shell
$ cmake --build . --target validateLab
[100%] Built target validate
Validation Successful
[100%] Built target validateLab
```



```shell
$ cmake --build . --target benchmarkLab
[100%] Built target lab
2026-06-04T00:12:19+08:00
Running ./lab
Run on (24 X 5400 MHz CPU s)
CPU Caches:
  L1 Data 48 KiB (x24)
  L1 Instruction 64 KiB (x24)
  L2 Unified 3072 KiB (x24)
  L3 Unified 36864 KiB (x1)
Load Average: 1.59, 0.91, 0.68
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
-----------------------------------------------------------
Benchmark                 Time             CPU   Iterations
-----------------------------------------------------------
bench1/real_time        319 ms          165 ms            9
[100%] Built target benchmarkLab
```



```shell
$ toplev.py --drilldown -v --no-desc -- ./lab
2026-06-04T00:21:16+08:00
Running ./lab
Run on (24 X 5400 MHz CPU s)
CPU Caches:
  L1 Data 48 KiB (x24)
  L1 Instruction 64 KiB (x24)
  L2 Unified 3072 KiB (x24)
  L3 Unified 36864 KiB (x1)
Load Average: 0.72, 0.93, 0.79
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
-----------------------------------------------------------
Benchmark                 Time             CPU   Iterations
-----------------------------------------------------------
bench1/real_time        334 ms          154 ms            2
# 5.1-full, 4 on Intel(R) Core(TM) Ultra 7 270K Plus [arl]
core FE               Frontend_Bound                      % Slots                        0.4  < [27.0%]
core BAD              Bad_Speculation                     % Slots                        0.0  < [27.0%]
core BE               Backend_Bound                       % Slots                       98.7    [27.0%]
core RET              Retiring                            % Slots                        0.9  < [27.0%]
core FE               Frontend_Bound.Fetch_Latency        % Slots                        0.0  < [27.0%]
core FE               Frontend_Bound.Fetch_Bandwidth      % Slots                        0.3  < [27.0%]
core BAD              Bad_Speculation.Branch_Mispredicts  % Slots                        0.0  < [27.0%]
core BAD              Bad_Speculation.Machine_Clears      % Slots                        0.0  < [27.0%]
core BE/Mem           Backend_Bound.Memory_Bound          % Slots                       95.7    [27.0%]<==
core BE/Core          Backend_Bound.Core_Bound            % Slots                        3.0  < [27.0%]
core RET              Retiring.Light_Operations           % Slots                        0.6  < [27.0%]
core RET              Retiring.Heavy_Operations           % Slots                        0.4  < [27.0%]
core MUX                                                  %                             27.00  
atom FE               Frontend_Bound                      % Slots                        0.0  < [27.0%]
atom FE               Frontend_Bound.IFetch_Latency       % Slots                        0.1  < [27.0%]
atom FE               Frontend_Bound.IFetch_Bandwidth     % Slots                        0.1  < [27.0%]
atom BAD              Bad_Speculation                     % Slots                        0.1  < [27.0%]
atom BAD              Bad_Speculation.Branch_Mispredicts  % Slots                        0.1  < [27.0%]
atom BAD              Bad_Speculation.Machine_Clears      % Slots                        0.0  < [27.0%]
atom BE               Backend_Bound                       % Slots                       99.2    [27.0%]
atom BE               Backend_Bound.Core_Bound            % Slots                       12.4    [27.0%]
atom BE               Backend_Bound.Resource_Bound        % Slots                       86.8    [27.0%]<==
atom RET              Retiring                            % Slots                        0.0  < [27.0%]
Run toplev --describe Memory_Bound^ to get more information on bottleneck for core
Add --run-sample to find locations
Adding --nodes '!+Memory_Bound*/3,+MUX' for breakdown.
Please make sure workload does not move between core types for drilldown
Run toplev --describe Resource_Bound^ to get more information on bottleneck for atom
Adding --nodes '!+Resource_Bound*/3' for breakdown.
Please make sure workload does not move between core types for drilldown
Rerunning workload
2026-06-04T00:21:17+08:00
Running ./lab
Run on (24 X 5400 MHz CPU s)
CPU Caches:
  L1 Data 48 KiB (x24)
  L1 Instruction 64 KiB (x24)
  L2 Unified 3072 KiB (x24)
  L3 Unified 36864 KiB (x1)
Load Average: 0.72, 0.93, 0.79
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
-----------------------------------------------------------
Benchmark                 Time             CPU   Iterations
-----------------------------------------------------------
bench1/real_time        304 ms          146 ms            2
core BE/Mem           Backend_Bound.Memory_Bound              % Slots                       94.9    [32.0%]
core BE/Mem           Backend_Bound.Memory_Bound.L1_Bound     % Stalls                       6.8  < [32.0%]
core BE/Mem           Backend_Bound.Memory_Bound.L2_Bound     % Stalls                       0.7  < [32.0%]
core BE/Mem           Backend_Bound.Memory_Bound.L3_Bound     % Stalls                      88.2    [32.0%]
core BE/Mem           Backend_Bound.Memory_Bound.DRAM_Bound   % Stalls                       0.0  < [32.0%]
core BE/Mem           Backend_Bound.Memory_Bound.Store_Bound  % Stalls                       0.0  < [32.0%]
core MUX                                                      %                             32.00  
atom BE               Backend_Bound.Resource_Bound                    % Slots                       86.8    [32.0%]
atom BE               Backend_Bound.Resource_Bound.Mem_Scheduler      % Slots                        0.0  < [32.0%]
atom BE               Backend_Bound.Resource_Bound.Non_Mem_Scheduler  % Slots                        0.0  < [32.0%]
atom BE               Backend_Bound.Resource_Bound.Register           % Slots                       86.6    [32.0%]
atom BE               Backend_Bound.Resource_Bound.Reorder_Buffer     % Slots                        0.0  < [32.0%]
atom BE               Backend_Bound.Resource_Bound.Serialization      % Slots                        0.1  < [32.0%]
```



```shell
perf c2c record ./lab
perf c2c report --stdio
```



---



```c++
#define CACHELINE_ALIGN alignas(64)
struct CACHELINE_ALIGN Accumulator {
    std::atomic<uint32_t> value = 0;
};
```



```shell
$ cmake --build . --target benchmarkLab
[ 33%] Building CXX object CMakeFiles/lab.dir/solution.cpp.o
[ 66%] Linking CXX executable lab
[100%] Built target lab
2026-06-04T00:28:37+08:00
Running ./lab
Run on (24 X 5400 MHz CPU s)
CPU Caches:
  L1 Data 48 KiB (x24)
  L1 Instruction 64 KiB (x24)
  L2 Unified 3072 KiB (x24)
  L3 Unified 36864 KiB (x1)
Load Average: 0.95, 0.87, 0.80
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
-----------------------------------------------------------
Benchmark                 Time             CPU   Iterations
-----------------------------------------------------------
bench1/real_time       20.9 ms         15.6 ms          136
[100%] Built target benchmarkLab
```

