// 子向量最大和

#include<stdio.h>

int max(int a, int b) {
    if (a > b)
        return a;
    else
        return b;
}

// 暴力求解
int brute_force(int x[]) {
    int n = sizeof(x) / sizeof(int);
    int maxsum = -0x3f3f3f3f;
    int sum = 0;
    for (int i = 0; i < n; i++) {
        for (int j = i; j < n; j++) {
            sum = 0;
            for (int k = i; k <= j; k++) {
                sum += x[k];
            }
            maxsum = max(maxsum, sum);
        }
    }
    return maxsum;
}

// 暴力求解优化版
int brute_force_opt(int x[]) {
    int n = sizeof(x) / sizeof(int);
    int maxsum = -0x3f3f3f3f;
    int sum = 0;
    for (int i = 0; i < n; i++) {
        sum = 0;
        for (int j = i; j < n; j++) {
            sum += x[j];
            maxsum = max(maxsum, sum);
        }
    }
    return maxsum;
}

// 分治法
int divide_conquer(int x[], int left, int right) {
    if (left == right) {
        return x[left];
    }
    int mid = (left + right) / 2;
    int leftmax = divide_conquer(x, left, mid);
    int rightmax = divide_conquer(x, mid + 1, right);

    int leftsum = -0x3f3f3f3f;
    int sum = 0;
    for (int i = mid; i >= left; i--) {
        sum += x[i];
        leftsum = max(leftsum, sum);
    }

    int rightsum = -0x3f3f3f3f;
    sum = 0;
    for (int i = mid + 1; i <= right; i++) {
        sum += x[i];
        rightsum = max(rightsum, sum);
    }

    return max(max(leftmax, rightmax), leftsum + rightsum);
}

// 线性算法
int linear(int x[]) {
    int n = sizeof(x) / sizeof(int);
    int maxsum = -0x3f3f3f3f;
    int sum = 0;
    for (int i = 0; i < n; i++) {
        sum += x[i];
        if (sum > maxsum) {
            maxsum = sum;
        }
        if (sum < 0) {
            sum = 0;
        }
    }
    return maxsum;
}

int main() {
    int x[] = {8, -33, 16, 9, -12, 45, 67};
    int ans = brute_force(x);
    printf("%d", ans);
}