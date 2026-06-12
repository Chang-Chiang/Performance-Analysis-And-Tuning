// 排序算法示例 (Sorting Algorithms)
//
// 本例实现三种基础排序算法：
//   1. 冒泡排序 (Bubble Sort)    — 时间复杂度 O(n²)
//   2. 插入排序 (Insertion Sort) — 时间复杂度 O(n²)
//   3. 选择排序 (Selection Sort) — 时间复杂度 O(n²)
//
// 算法比较:
//   算法       最好    最坏    平均    稳定性
//   冒泡排序   O(n)    O(n²)   O(n²)   稳定
//   插入排序   O(n)    O(n²)   O(n²)   稳定
//   选择排序   O(n²)   O(n²)   O(n²)   不稳定
//
// 编译命令:
//   clang++ -O1 sort.cpp -o sort

#include <assert.h>
#include <chrono>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 冒泡排序：相邻元素比较交换，每轮将最大元素移到末尾
void bubble_sort(int a[], int n) {
    for (int j = 0; j < n - 1; j++) {
        for (int i = 0; i < n - 1 - j; i++) {
            if (a[i] > a[i + 1]) {
                int temp = a[i];
                a[i]     = a[i + 1];
                a[i + 1] = temp;
            }
        }
    }
}

// 插入排序：将元素插入到已排序序列的正确位置
void insert_sort(int* arr, size_t size) {
    assert(arr);
    for (int idx = 1; idx <= size - 1; idx++) {
        int end  = idx;
        int temp = arr[end];
        while (end > 0 && temp < arr[end - 1]) {
            arr[end] = arr[end - 1];
            end--;
        }
        arr[end] = temp;
    }
}

// 选择排序：每轮选择最小元素放到已排序序列末尾
void select_sort(int a[], int len) {
    for (int i = 0; i < len - 1; i++) {
        int minIndex = i;
        for (int j = i + 1; j < len; j++) {
            if (a[j] < a[minIndex]) {
                minIndex = j;
            }
        }
        if (minIndex != i) {
            int temp = a[i];
            a[i]     = a[minIndex];
            a[minIndex] = temp;
        }
    }
}

// 生成随机数组
void generate_random_array(int arr[], int n) {
    srand(42);  // 固定随机种子，保证每次测试数据相同
    for (int i = 0; i < n; i++) {
        arr[i] = rand() % 10000;
    }
}

// 测试排序算法性能
void benchmark(const char* name, void (*sort_func)(int*, int), int n) {
    int* arr = new int[n];
    generate_random_array(arr, n);

    auto start = std::chrono::high_resolution_clock::now();
    sort_func(arr, n);
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    printf("%-12s n=%6d  用时 %8ld μs\n", name, n, duration.count());

    delete[] arr;
}

int main() {
    int sizes[] = {1000, 5000, 10000};

    printf("排序算法性能比较:\n");
    printf("----------------------------------------\n");

    for (int i = 0; i < 3; i++) {
        int n = sizes[i];
        benchmark("冒泡排序", bubble_sort, n);
        benchmark("插入排序", (void (*)(int*, int))insert_sort, n);
        benchmark("选择排序", select_sort, n);
        printf("----------------------------------------\n");
    }

    return 0;
}
