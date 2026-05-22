```c++
for (int c = 0; c < width; c++)
{
    for (int r = radius; r < height - radius; r++)
    {
		// Accumulation
      	int dot = 0;
        for (int i = 0; i < radius + 1 + radius; i++)
        {
        	dot += input[(r - radius + i) * width + c] * kernel[i];
        }

        // Fast shift instead of division
        int value = (dot + rounding) >> shift;
        output[r * width + c] = static_cast<uint8_t>(value);
    }
}
```

```shell
$ cmake --build . --target benchmarkLab
[100%] Built target lab
2026-05-20T20:44:58+08:00
Running ./lab
Run on (12 X 4100 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x6)
  L1 Instruction 32 KiB (x6)
  L2 Unified 256 KiB (x6)
  L3 Unified 9216 KiB (x1)
Load Average: 1.25, 1.61, 1.89
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
***WARNING*** ASLR is enabled, the results may have unreproducible noise in them.
-----------------------------------------------------
Benchmark           Time             CPU   Iterations
-----------------------------------------------------
bench1      282254786 ns    282249133 ns            9
[100%] Built target benchmarkLab
```

```shell
```







---



```c++
for (int c = 0; c < width; c++) {

    int dot[height - radius];

    for (int r = radius; r < height - radius; r++) {
        // Accumulation
        dot[r] = 0;
        for (int i = 0; i < radius + 1 + radius; i++) {
            dot[r] += input[(r - radius + i) * width + c] * kernel[i];
        }

        // Fast shift instead of division
        int value = (dot[r] + rounding) >> shift;
        output[r * width + c] = static_cast<uint8_t>(value);
    }
}
```



```shell
$ cmake --build . --target validateLab
[ 33%] Building CXX object CMakeFiles/validate.dir/solution.cpp.o
[ 66%] Linking CXX executable validate
[100%] Built target validate
Validation Successful
[100%] Built target validateLab
```



---



```c++
for (int c = 0; c < width; c++) {

    int dot[height - radius];

    for (int r = radius; r < height - radius; r++) {
        // Accumulation
        dot[r] = 0;
        for (int i = 0; i < radius + 1 + radius; i++) {
            dot[r] += input[(r - radius + i) * width + c] * kernel[i];
        }
    }

    for (int r = radius; r < height - radius; r++) {
        // Fast shift instead of division
        int value = (dot[r] + rounding) >> shift;
        output[r * width + c] = static_cast<uint8_t>(value);
    }
}
```



```shell
$ cmake --build . --target validateLab
[ 33%] Building CXX object CMakeFiles/validate.dir/solution.cpp.o
[ 66%] Linking CXX executable validate
[100%] Built target validate
Validation Successful
[100%] Built target validateLab
```



---



```c++
int *dot = new int[(height - radius) * width];
for (int c = 0; c < width; c++) {

    for (int r = radius; r < height - radius; r++) {
        // Accumulation
        dot[r * width + c] = 0;
        for (int i = 0; i < radius + 1 + radius; i++) {
            dot[r * width + c] += input[(r - radius + i) * width + c] * kernel[i];
        }
    }

    for (int r = radius; r < height - radius; r++) {
        // Fast shift instead of division
        int value = (dot[r * width + c] + rounding) >> shift;
        output[r * width + c] = static_cast<uint8_t>(value);
    }
}
```



```shell
$ cmake --build . --target validateLab
[ 33%] Building CXX object CMakeFiles/validate.dir/solution.cpp.o
[ 66%] Linking CXX executable validate
[100%] Built target validate
Validation Successful
[100%] Built target validateLab
```



---



```c++
int *dot = new int[(height - radius) * width];
for (int c = 0; c < width; c++) {

    for (int r = radius; r < height - radius; r++) {
        // Accumulation
        dot[r * width + c] = 0;
        for (int i = 0; i < radius + 1 + radius; i++) {
            dot[r * width + c] += input[(r - radius + i) * width + c] * kernel[i];
        }
    }
}

for (int c = 0; c < width; c++) {
    for (int r = radius; r < height - radius; r++) {
        // Fast shift instead of division
        int value = (dot[r * width + c] + rounding) >> shift;
        output[r * width + c] = static_cast<uint8_t>(value);
    }
}
```



```shell
$ cmake --build . --target validateLab
[ 33%] Building CXX object CMakeFiles/validate.dir/solution.cpp.o
[ 66%] Linking CXX executable validate
[100%] Built target validate
Validation Successful
[100%] Built target validateLab
```



---



```c++
int *dot = new int[(height - radius) * width];
for (int r = radius; r < height - radius; r++) {
    for (int c = 0; c < width; c++) {
        // Accumulation
        dot[r * width + c] = 0;
        for (int i = 0; i < radius + 1 + radius; i++) {
            dot[r * width + c] += input[(r - radius + i) * width + c] * kernel[i];
        }
    }
}

for (int r = radius; r < height - radius; r++) {
    for (int c = 0; c < width; c++) {
        // Fast shift instead of division
        int value = (dot[r * width + c] + rounding) >> shift;
        output[r * width + c] = static_cast<uint8_t>(value);
    }
}
```



```shell
$ cmake --build . --target validateLab
[ 33%] Building CXX object CMakeFiles/validate.dir/solution.cpp.o
[ 66%] Linking CXX executable validate
[100%] Built target validate
Validation Successful
[100%] Built target validateLab
```



---



```c++
int *dot = new int[(height - radius) * width];
for (int r = radius; r < height - radius; r++) {
    for (int c = 0; c < width; c++) {
        dot[r * width + c] = 0;
    }
    
    // Accumulation
    for (int c = 0; c < width; c++) {
        for (int i = 0; i < radius + 1 + radius; i++) {
            dot[r * width + c] += input[(r - radius + i) * width + c] * kernel[i];
        }
    }
}

for (int r = radius; r < height - radius; r++) {
    for (int c = 0; c < width; c++) {
        // Fast shift instead of division
        int value = (dot[r * width + c] + rounding) >> shift;
        output[r * width + c] = static_cast<uint8_t>(value);
    }
}
```



```shell
$ cmake --build . --target validateLab
[ 33%] Building CXX object CMakeFiles/validate.dir/solution.cpp.o
[ 66%] Linking CXX executable validate
[100%] Built target validate
Validation Successful
[100%] Built target validateLab
```



---



```c++
int *dot = new int[(height - radius) * width];
for (int r = radius; r < height - radius; r++) {
    for (int c = 0; c < width; c++) {
        dot[r * width + c] = 0;
    }
}
// Accumulation
for (int r = radius; r < height - radius; r++) {
    for (int c = 0; c < width; c++) {
        for (int i = 0; i < radius + 1 + radius; i++) {
            dot[r * width + c] += input[(r - radius + i) * width + c] * kernel[i];
        }
    }
}

for (int r = radius; r < height - radius; r++) {
    for (int c = 0; c < width; c++) {
        // Fast shift instead of division
        int value = (dot[r * width + c] + rounding) >> shift;
        output[r * width + c] = static_cast<uint8_t>(value);
    }
}
```



```shell
$ cmake --build . --target validateLab
[ 33%] Building CXX object CMakeFiles/validate.dir/solution.cpp.o
[ 66%] Linking CXX executable validate
[100%] Built target validate
Validation Successful
[100%] Built target validateLab
```



---



```c++
int *dot = new int[(height - radius) * width];
for (int r = radius; r < height - radius; r++) {
    for (int c = 0; c < width; c++) {
        dot[r * width + c] = 0;
    }
}
// Accumulation
for (int r = radius; r < height - radius; r++) {
    for (int i = 0; i < radius + 1 + radius; i++) {
        for (int c = 0; c < width; c++) {
            dot[r * width + c] += input[(r - radius + i) * width + c] * kernel[i];
        }
    }
}

for (int r = radius; r < height - radius; r++) {
    for (int c = 0; c < width; c++) {
        // Fast shift instead of division
        int value = (dot[r * width + c] + rounding) >> shift;
        output[r * width + c] = static_cast<uint8_t>(value);
    }
}
```



```shell
$ cmake --build . --target validateLab
[ 33%] Building CXX object CMakeFiles/validate.dir/solution.cpp.o
[ 66%] Linking CXX executable validate
[100%] Built target validate
Validation Successful
[100%] Built target validateLab
```



---



```c++
int *dot = new int[(height - radius) * width];
for (int r = radius; r < height - radius; r++) {
    for (int c = 0; c < width; c++) {
        dot[r * width + c] = 0;
    }

    // Accumulation
    for (int i = 0; i < radius + 1 + radius; i++) {
        for (int c = 0; c < width; c++) {
            dot[r * width + c] += input[(r - radius + i) * width + c] * kernel[i];
        }
    }

    for (int c = 0; c < width; c++) {
        // Fast shift instead of division
        int value = (dot[r * width + c] + rounding) >> shift;
        output[r * width + c] = static_cast<uint8_t>(value);
    }
}
```



```shell
$ cmake --build . --target validateLab
[ 33%] Building CXX object CMakeFiles/validate.dir/solution.cpp.o
[ 66%] Linking CXX executable validate
[100%] Built target validate
Validation Successful
[100%] Built target validateLab
```



---



```c++
for (int r = radius; r < height - radius; r++) {
    int dot[width];
    for (int c = 0; c < width; c++) {
        dot[c] = 0;
    }

    // Accumulation
    for (int i = 0; i < radius + 1 + radius; i++) {
        for (int c = 0; c < width; c++) {
            dot[c] += input[(r - radius + i) * width + c] * kernel[i];
        }
    }

    for (int c = 0; c < width; c++) {
        // Fast shift instead of division
        int value = (dot[c] + rounding) >> shift;
        output[r * width + c] = static_cast<uint8_t>(value);
    }
}
```



```shell
$ cmake --build . --target validateLab
[ 33%] Building CXX object CMakeFiles/validate.dir/solution.cpp.o
[ 66%] Linking CXX executable validate
[100%] Built target validate
Validation Successful
[100%] Built target validateLab
```



```shell
$ cmake --build . --target benchmarkLab
[ 33%] Building CXX object CMakeFiles/lab.dir/solution.cpp.o
[ 66%] Linking CXX executable lab
[100%] Built target lab
2026-05-22T22:42:30+08:00
Running ./lab
Run on (12 X 4100 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x6)
  L1 Instruction 32 KiB (x6)
  L2 Unified 256 KiB (x6)
  L3 Unified 9216 KiB (x1)
Load Average: 1.37, 1.03, 0.70
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
***WARNING*** ASLR is enabled, the results may have unreproducible noise in them.
-----------------------------------------------------
Benchmark           Time             CPU   Iterations
-----------------------------------------------------
bench1       69646421 ns     69523585 ns           39
[100%] Built target benchmarkLab
```

