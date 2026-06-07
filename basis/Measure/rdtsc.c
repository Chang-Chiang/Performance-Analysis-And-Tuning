#include <stdio.h>
#include <unistd.h>

#define n 100
#define m 10
int x, i, j;

// 执行 rdtsc 指令 → 从 EDX:EAX 取出 64 位时间戳 → 拼成 unsigned long 返回
unsigned long rpcc() {

    // 存储时间戳
    unsigned long result;

    // 存储 rdtsc 指令返回的高 32 位和低 32 位
    unsigned hi, lo;

    // 使用内联汇编执行 rdtsc 指令，并将结果分别存储在 lo 和 hi 中
    // "volatile" 关键字告诉编译器不要优化这段代码，
    // 否则编译器可能认为这条指令没有副作用而将其删除，或者将其移到循环外面，从而导致测量不准确
    asm volatile("rdtsc" : "=a"(lo), "=d"(hi));

    // 将高 32 位和低 32 位拼接成一个 64 位的时间戳
    result = ((unsigned long long)lo) | (((unsigned long long)hi) << 32);

    return result;
}

void fun(int a) {
    for (i = 0; i < n; i++) {
        x = (a / 4) + i;
    }
}

int main() {
    unsigned long b[m], start, end, k;
    for (j = 0; j < 10; j++) {
        start = rpcc();
        fun(16);
        end  = rpcc();
        b[j] = end - start;
        k += b[j];
    }
    printf("time = %ld\n", k / m);
}