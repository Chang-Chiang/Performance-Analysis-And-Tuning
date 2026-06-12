// 最大子数组和 (Maximum Subarray Sum)
//
// 问题描述：
//   给定一个整数数组，找到一个连续子数组，使得子数组的和最大。
//   例如：[8, -33, 16, 9, -12, 45, 67] 的最大子数组和为 125（[16, 9, -12, 45, 67]）
//
// 算法比较：
//   算法         时间复杂度   空间复杂度
//   暴力求解     O(n³)       O(1)
//   暴力求解优化 O(n²)       O(1)
//   分治法       O(n log n)  O(log n)
//   线性算法     O(n)        O(1)
//
// 编译命令：
//   clang++ -O1 max_sum.cpp -o max_sum

#include <stdio.h>
#include <stdlib.h>

#include <chrono>

// 辅助函数：返回两个数中的较大值
int max(int a, int b) { return (a > b) ? a : b; }

// 暴力求解：遍历所有子数组，计算每个子数组的和
// 时间复杂度：O(n³)
int brute_force(int x[], int n) {
    int maxsum = -0x3f3f3f3f;
    for (int i = 0; i < n; i++) {
        for (int j = i; j < n; j++) {
            int sum = 0;
            for (int k = i; k <= j; k++) {
                sum += x[k];
            }
            maxsum = max(maxsum, sum);
        }
    }
    return maxsum;
}

// 暴力求解优化版：在遍历过程中累加和，避免重复计算
// 时间复杂度：O(n²)
int brute_force_opt(int x[], int n) {
    int maxsum = -0x3f3f3f3f;
    for (int i = 0; i < n; i++) {
        int sum = 0;
        for (int j = i; j < n; j++) {
            sum += x[j];
            maxsum = max(maxsum, sum);
        }
    }
    return maxsum;
}

// 分治法：将数组分成两半，分别求解，再合并结果
// 时间复杂度：O(n log n)
int divide_conquer(int x[], int left, int right) {
    if (left == right) {
        return x[left];
    }

    int mid = (left + right) / 2;

    // 递归求解左右两部分的最大子数组和
    int leftmax  = divide_conquer(x, left, mid);
    int rightmax = divide_conquer(x, mid + 1, right);

    // 计算跨越中点的最大子数组和
    int leftsum = -0x3f3f3f3f;
    int sum     = 0;
    for (int i = mid; i >= left; i--) {
        sum += x[i];
        leftsum = max(leftsum, sum);
    }

    int rightsum = -0x3f3f3f3f;
    sum          = 0;
    for (int i = mid + 1; i <= right; i++) {
        sum += x[i];
        rightsum = max(rightsum, sum);
    }

    return max(max(leftmax, rightmax), leftsum + rightsum);
}

// 线性算法（Kadane 算法）：动态规划思想
// 时间复杂度：O(n)
int linear(int x[], int n) {
    int maxsum = -0x3f3f3f3f;
    int sum    = 0;
    for (int i = 0; i < n; i++) {
        sum += x[i];
        if (sum > maxsum) {
            maxsum = sum;
        }
        if (sum < 0) {
            sum = 0; // 如果当前和为负，重新开始累加
        }
    }
    return maxsum;
}

// 生成随机数组
void generate_random_array(int arr[], int n) {
    srand(42);
    for (int i = 0; i < n; i++) {
        arr[i] = rand() % 2001 - 1000; // -1000 到 1000 的随机数
    }
}

// 测试算法性能
void benchmark(const char* name, int (*func)(int*, int), int arr[], int n) {
    auto start  = std::chrono::high_resolution_clock::now();
    int  result = func(arr, n);
    auto end    = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    printf("│ %-16s │ %6d │ %10d │ %12ld │\n", name, n, result, duration.count());
}

int main() {
    int sizes[] = {1000, 5000, 10000};

    printf("Maximum Subarray Sum Benchmark:\n");
    printf("┌──────────────────┬────────┬────────────┬──────────────┐\n");
    printf("│ Algorithm        │      n │    Max Sum │    Time (us) │\n");
    printf("├──────────────────┼────────┼────────────┼──────────────┤\n");

    for (int i = 0; i < 3; i++) {
        int  n   = sizes[i];
        int* arr = new int[n];
        generate_random_array(arr, n);

        benchmark("Brute Force", brute_force, arr, n);
        benchmark("Brute Force+", brute_force_opt, arr, n);
        benchmark(
            "Divide&Conquer", [](int* x, int n) { return divide_conquer(x, 0, n - 1); }, arr, n);
        benchmark("Kadane", linear, arr, n);

        printf("├──────────────────┼────────┼────────────┼──────────────┤\n");
        delete[] arr;
    }

    return 0;
}
