// ============================================================================
// rdtsc.c · 周期级性能测量 (x86 时间戳计数器 TSC)
// ============================================================================
//
// 目的：
//   当代码段耗时 < 微秒、time/chrono 分辨率不够时, 用 rdtsc 直接数 CPU 周期。
//   rpcc() 读 TSC(每周期 +1); 换算时间 = cycles / 主频 (3GHz → 1 周期 ≈ 0.33ns)。
//
// ⚠️ 本文件是「测量陷阱」活教材 —— DCE 死代码消除 (见 30 测量纪律手册 · 陷阱 4)：
//   fun() 里 for 循环写的是全局 x、i, 而全程序没有任何地方读它们。
//   于是编译器在 -O2 有权把整个循环「折叠成闭式解」(只算末值), 循环根本不执行。
//   你以为在测 100 次循环, 其实测的是两次 rdtsc 调用自身的开销 → 数字全废。
//
//   本机 (gcc 15.2 / 混合架构大核) 实测 fun() 平均周期：
//     -------------------------------------------------------------
//     编译方式                    循环命运               结果(周期)
//     -------------------------------------------------------------
//     原文件 -O0                  原样跑, 每迭代做除法   ~209  (假)
//     原文件 -O2                  折叠成闭式解, 没跑     ~22   (假: DCE)
//     修正 -O2 (加 volatile sink) 真实执行, 被合法优化   ~72   (真)
//     -------------------------------------------------------------
//     -O0 假在「你上线的不是它」; -O2 假在「活根本没干」; 72 才是生产真实成本。
//     注：绝对值强依赖硬件/频率, 换机器须重测 (测量纪律 #3)。
//
// 正确测法 (让「想测的计算」逃过优化, 逃过 DCE, 任选其一)：
//   ① volatile 落点:  volatile int x;  // 每次写都必须发生, 循环无法被折叠
//   ② 把结果 return / 累加进 volatile sink, 最后 printf 出来
//   ③ 优化屏障:       asm volatile("" :: "r"(x) : "memory");
//   另：现代乱序 CPU 上单独 rdtsc 不序列化, 计时起点建议用 "lfence;rdtsc"。
//
// 度量纪律：微基准要按「生产的 -O 级别 (-O2/-O3 -march=native)」测, 同时给结果加防消除出口;
//   别拿一个孤零零的数字下结论 —— 用反汇编 / -Rpass 让编译器「交代」(见 04 编译器概述)。
//
// 编译命令：
//   gcc -O0 rdtsc.c -o rdtsc_O0 && ./rdtsc_O0                        # ~209, 不代表生产
//   gcc -O2 rdtsc.c -o rdtsc_O2 && ./rdtsc_O2                        # ~22,  循环被 DCE
//   objdump -d -M intel rdtsc_O2 | sed -n '/<fun>:/,/^$/p'           # 亲眼验证循环没了
//
// 延伸阅读：docs/notes/01-性能度量指标.md、docs/notes/30-测量纪律手册.md
// ============================================================================

#include <stdio.h>
#include <unistd.h>

#define n 100
#define m 10

// 防止 DCE
volatile int x; 
// int x;
int i, j;

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

    // 现代乱序 CPU 上单独 rdtsc 不阻止后续指令重排到它之前,
    // 严谨计时起点应用 "lfence;rdtsc", 终点 "rdtsc;lfence" (本例只演示机制, 从简)。
    // asm volatile("lfence\n\trdtsc" : "=a"(lo), "=d"(hi)); 

    // 将高 32 位和低 32 位拼接成一个 64 位的时间戳
    result = ((unsigned long long)lo) | (((unsigned long long)hi) << 32);

    return result;
}

void fun(int a) {

    for (i = 0; i < n; i++) {
        x = (a / 4) + i;

        // 防止 x 被优化
        // asm volatile("" :: "r"(x) : "memory");
    }
}

int main() {
    unsigned long b[m], start, end;
    unsigned long k = 0;
    for (j = 0; j < 10; j++) {
        start = rpcc();
        fun(16);
        end  = rpcc();
        b[j] = end - start;
        k += b[j];
    }
    printf("time = %ld\n", k / m);
}