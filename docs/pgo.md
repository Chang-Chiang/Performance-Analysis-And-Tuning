

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
2026-06-04T07:40:50+08:00
Running ./lab
Run on (24 X 5400 MHz CPU s)
CPU Caches:
  L1 Data 48 KiB (x24)
  L1 Instruction 64 KiB (x24)
  L2 Unified 3072 KiB (x24)
  L3 Unified 36864 KiB (x1)
Load Average: 0.35, 0.52, 0.62
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
-----------------------------------------------------
Benchmark           Time             CPU   Iterations
-----------------------------------------------------
bench1           2987 ms         2986 ms            1
[100%] Built target benchmarkLab
```



---



```cmake
cmake_minimum_required(VERSION 3.3)

project(lab)

include_directories(lua)
file(GLOB EXT_LAB_srcs lua/*.c)
file(GLOB EXT_VALIDATE_srcs lua/*.c)

set(CMAKE_C_FLAGS "-fprofile-instr-generate" ${CMAKE_C_FLAGS})

set(VALIDATE_ARGS "${CMAKE_CURRENT_SOURCE_DIR}/reference_output.txt")

string(REGEX MATCH "^(.*)[\\/]labs[\\/].*$" repo "${CMAKE_CURRENT_SOURCE_DIR}")
include(${CMAKE_MATCH_1}/tools/labs.cmake)

```

