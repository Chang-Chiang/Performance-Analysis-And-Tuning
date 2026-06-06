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
2026-06-04T00:48:59+08:00
Running ./lab
Run on (24 X 5400 MHz CPU s)
CPU Caches:
  L1 Data 48 KiB (x24)
  L1 Instruction 64 KiB (x24)
  L2 Unified 3072 KiB (x24)
  L3 Unified 36864 KiB (x1)
Load Average: 0.79, 0.48, 0.56
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
-----------------------------------------------------
Benchmark           Time             CPU   Iterations
-----------------------------------------------------
bench1            519 ms          519 ms            5
[100%] Built target benchmarkLab
```



---



```cmake
cmake_minimum_required(VERSION 3.5)

project(lab)

set(VALIDATE_ARGS "${CMAKE_CURRENT_SOURCE_DIR}/golden.ppm")

set(CMAKE_CXX_FLAGS "-flto ${CMAKE_CXX_FLAGS}")

string(REGEX MATCH "^(.*)[\\/]misc[\\/].*$" repo "${CMAKE_CURRENT_SOURCE_DIR}")
include(${CMAKE_MATCH_1}/tools/labs.cmake)
```



```shell
$ cmake --build . --target validateLab
[100%] Built target validate
Validation Successful
[100%] Built target validateLab
```



```shell
$ cmake --build . --target benchmarkLab
[ 11%] Building CXX object CMakeFiles/lab.dir/bench.cpp.o
[ 22%] Building CXX object CMakeFiles/lab.dir/ao.cpp.o
[ 33%] Building CXX object CMakeFiles/lab.dir/ao_helpers.cpp.o
[ 44%] Building CXX object CMakeFiles/lab.dir/ao_init.cpp.o
[ 55%] Building CXX object CMakeFiles/lab.dir/ao_intersect.cpp.o
[ 66%] Building CXX object CMakeFiles/lab.dir/ao_occlusion.cpp.o
[ 77%] Building CXX object CMakeFiles/lab.dir/ao_orthoBasis.cpp.o
[ 88%] Building CXX object CMakeFiles/lab.dir/ao_render.cpp.o
[100%] Linking CXX executable lab
[100%] Built target lab
2026-06-04T00:51:02+08:00
Running ./lab
Run on (24 X 5400 MHz CPU s)
CPU Caches:
  L1 Data 48 KiB (x24)
  L1 Instruction 64 KiB (x24)
  L2 Unified 3072 KiB (x24)
  L3 Unified 36864 KiB (x1)
Load Average: 0.45, 0.46, 0.54
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
-----------------------------------------------------
Benchmark           Time             CPU   Iterations
-----------------------------------------------------
bench1            344 ms          344 ms            8
[100%] Built target benchmarkLab
```

