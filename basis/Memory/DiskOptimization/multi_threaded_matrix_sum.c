/**
 * 多线程矩阵求和示例
 *
 * 原理：
 * 多线程操作可以充分利用多核处理器的并行性，提高计算密集型任务的性能。
 * 本例展示了使用 pthread 库进行多线程矩阵求和。
 *
 * 优化策略：
 * 1. 将矩阵按列分割，每个线程负责一部分列的求和
 * 2. 使用结构体传递线程参数
 * 3. 使用 pthread_join 等待所有线程完成
 * 4. 最后将各线程的结果相加得到最终结果
 *
 * 编译指令（需要 pthread 库）：
 * gcc -O2 -lpthread -o multi_threaded_matrix_sum multi_threaded_matrix_sum.c
 *
 * 运行：
 * ./multi_threaded_matrix_sum
 */

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>

#define NUM_THREADS 5
#define ROWS 1000
#define COLS 5000

/* 全局矩阵 */
int arr[ROWS][COLS];

/* 线程参数结构体 */
typedef struct {
    int first;   /* 起始列 */
    int last;    /* 结束列 */
    int result;  /* 求和结果 */
} MY_ARGS;

/* 线程函数 */
void* myfunc(void* args) {
    int i, j;
    int s = 0;
    MY_ARGS* my_args = (MY_ARGS*)args;

    /* 计算指定列范围的和 */
    for (i = 0; i < ROWS; i++) {
        for (j = my_args->first; j < my_args->last; j++) {
            s += arr[i][j];
        }
    }

    /* 保存结果 */
    my_args->result = s;
    return NULL;
}

int main() {
    int i, j;
    pthread_t threads[NUM_THREADS];
    MY_ARGS args[NUM_THREADS];
    int cols_per_thread = COLS / NUM_THREADS;

    /* 初始化随机数生成器 */
    srand(time(NULL));

    /* 初始化矩阵 */
    for (i = 0; i < ROWS; i++) {
        for (j = 0; j < COLS; j++) {
            arr[i][j] = rand() % 50;
        }
    }

    /* 创建线程 */
    for (i = 0; i < NUM_THREADS; i++) {
        args[i].first = i * cols_per_thread;
        args[i].last = (i == NUM_THREADS - 1) ? COLS : (i + 1) * cols_per_thread;
        args[i].result = 0;

        if (pthread_create(&threads[i], NULL, myfunc, &args[i]) != 0) {
            perror("创建线程失败");
            exit(1);
        }
    }

    /* 等待所有线程完成 */
    for (i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    /* 汇总结果 */
    int total_sum = 0;
    for (i = 0; i < NUM_THREADS; i++) {
        total_sum += args[i].result;
    }

    printf("矩阵维度：%d x %d\n", ROWS, COLS);
    printf("线程数量：%d\n", NUM_THREADS);
    printf("总和：%d\n", total_sum);

    return 0;
}

/**
 * 多线程优化要点：
 * 1. 任务分割：将大任务分割成多个小任务，分配给不同线程
 * 2. 负载均衡：确保每个线程的工作量大致相等
 * 3. 同步开销：尽量减少线程间的同步操作
 * 4. 数据局部性：每个线程访问的数据应该尽量连续
 *
 * 性能分析：
 * - 理想情况下，N 个线程可以将性能提升 N 倍
 * - 实际性能受以下因素影响：
 *   1. 线程创建和销毁的开销
 *   2. 线程同步的开销
 *   3. 共享资源的竞争
 *   4. 缓存一致性开销
 *
 * 适用场景：
 * 1. 计算密集型任务
 * 2. 数据可以自然分割
 * 3. 线程间不需要频繁通信
 * 4. 有多核处理器可用
 */
