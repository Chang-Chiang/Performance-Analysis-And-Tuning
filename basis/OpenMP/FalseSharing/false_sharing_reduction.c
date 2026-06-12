/**
 * 使用 reduction 避免伪共享示例
 *
 * 原理：
 * 使用 OpenMP 的 reduction 子句，让每个线程维护变量的私有副本，
 * 最后进行归约操作，从而避免伪共享问题。
 *
 * 优化前：
 * - 使用共享数组 sum[NTHREADS]
 * - 多个线程访问不同元素导致伪共享
 *
 * 优化后：
 * - 使用 reduction(+:sum) 子句
 * - 每个线程有 sum 的私有副本
 * - 最后自动进行加法归约
 *
 * 编译指令：
 * gcc -O2 -fopenmp -o false_sharing_reduction false_sharing_reduction.c
 *
 * 运行：
 * ./false_sharing_reduction
 */

#include <stdio.h>
#include <omp.h>

#define NTHREADS 4
static long num_steps = 1000000;
double step;

int main() {
    int i, j;
    double pi, start_time, used_time;
    double sum = 0.0;  /* 单个变量，使用 reduction 保护 */

    step = 1.0 / (double)num_steps;

    start_time = omp_get_wtime();
    #pragma omp parallel num_threads(NTHREADS)
    {
        int i;
        int id = omp_get_thread_num();
        double x;

        /* reduction(+:sum) 子句：
         * 1. 每个线程创建 sum 的私有副本
         * 2. 私有副本初始化为 0（加法的单位元）
         * 3. 循环结束后，所有私有副本进行加法归约
         * 4. 归约结果存储到主线程的 sum 变量 */
        #pragma omp for reduction(+:sum)
        for (i = 0; i < num_steps; i++) {
            x = (i + 0.5) * step;
            sum += 4.0 / (1.0 + x * x);
        }
    }

    pi = step * sum;

    used_time = omp_get_wtime() - start_time;
    printf("reduction 版本：pi = %15.13f, runtime = %lf seconds\n", pi, used_time);

    return 0;
}

/**
 * reduction 子句详解：
 *
 * 语法：reduction(operator:variable)
 *
 * 支持的操作符：
 * - +：加法
 * - *：乘法
 * - -：减法
 * - &：按位与
 * - |：按位或
 * - ^：按位异或
 * - &&：逻辑与
 * - ||：逻辑或
 * - max：最大值
 * - min：最小值
 *
 * 工作原理：
 * 1. 每个线程创建变量的私有副本
 * 2. 私有副本初始化为操作符的单位元
 *    - 加法：0
 *    - 乘法：1
 *    - 最小值：最大可能值
 *    - 最大值：最小可能值
 * 3. 线程使用私有副本进行计算
 * 4. 循环结束后，所有私有副本进行归约操作
 * 5. 归约结果存储到主线程的变量
 *
 * 优点：
 * 1. 避免伪共享问题
 * 2. 代码简洁，不需要手动管理私有副本
 * 3. 编译器可以进行优化
 * 4. 支持多种归约操作
 *
 * 与手动实现的对比：
 * - 手动实现：需要创建数组、手动汇总
 * - reduction：编译器自动处理，代码更简洁
 */
