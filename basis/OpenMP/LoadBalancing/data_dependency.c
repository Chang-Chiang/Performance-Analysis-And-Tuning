/**
 * 数据依赖示例 - 求解偏微分方程
 *
 * 原理：
 * 在求解偏微分方程（如 Jacobi 迭代）时，存在数据依赖：
 * - 新值的计算依赖于旧值
 * - 相邻元素之间存在依赖关系
 * - 不能简单地并行化
 *
 * 本例展示了三种处理数据依赖的方法：
 * 1. 串行版本：单线程计算
 * 2. 按列并行：每列独立计算，但列之间有依赖
 * 3. 分块并行：将计算分块，减少同步次数
 *
 * 编译指令：
 * gcc -O2 -fopenmp -o data_dependency data_dependency.c
 *
 * 运行：
 * ./data_dependency
 */

#include <stdio.h>
#include <omp.h>

#define N1 10000
#define N2 10000
#define min(a,b) ((a)<(b)?(a):(b))

double a[N1][N2];

/**
 * 初始化数组
 */
void init_array() {
    for (int i = 0; i < N1; i++) {
        for (int j = 0; j < N2; j = j + 4)
            a[i][j] = 3;
        for (int j = 1; j < N2; j = j + 4)
            a[i][j] = 5;
        for (int j = 2; j < N2; j = j + 4)
            a[i][j] = 4;
        for (int j = 3; j < N2; j = j + 4)
            a[i][j] = 8;
        for (int j = 4; j < N2; j = j + 5)
            a[i][j] = 7;
    }
}

/**
 * 串行版本：按列顺序计算
 * 每列的计算依赖于前一列的结果
 */
void jacobi_serial() {
    double start_time, end_time, used_time;

    printf("=== 串行版本 ===\n");
    start_time = omp_get_wtime();

    /* 按列顺序计算
     * 每列的计算依赖于前一列的结果
     * 不能并行化 */
    for (int i = 1; i < N1 - 1; i++) {
        for (int j = 1; j < N2 - 1; j++) {
            a[i][j] = 0.25 * (a[i-1][j] + a[i][j-1] + a[i+1][j] + a[i][j+1]);
        }
    }

    end_time = omp_get_wtime();
    used_time = end_time - start_time;

    /* 计算校验和 */
    double sum = 0;
    for (int i = 0; i < N1; i++)
        for (int j = 0; j < N2; j++)
            sum += a[i][j];

    printf("sum=%lf, used_time=%lf seconds\n\n", sum, used_time);
}

/**
 * 按列并行版本：每列使用多线程计算
 * 列之间有依赖，但列内可以并行
 */
void jacobi_column_parallel() {
    int isync[256], mthreadnum, iam;
    #pragma omp threadprivate(mthreadnum, iam)

    /* 同步函数：等待前一列完成 */
    auto sync_left = [&]() {
        int neighbour;
        if (iam > 0 && iam <= mthreadnum) {
            neighbour = iam - 1;
            while (isync[neighbour] == 0) {
                #pragma omp flush(isync)
            }
            isync[neighbour] = 0;
            #pragma omp flush(isync, a)
        }
    };

    /* 同步函数：通知下一列可以开始 */
    auto sync_right = [&]() {
        if (iam < mthreadnum) {
            while (isync[iam] == 1) {
                #pragma omp flush(isync)
            }
            #pragma omp flush(isync, a)
            isync[iam % (mthreadnum - 1)] = 1;
            #pragma omp flush(isync)
        }
    };

    double start_time, end_time, used_time;

    printf("=== 按列并行版本 ===\n");
    start_time = omp_get_wtime();

    isync[0] = 1;
    #pragma omp parallel default(shared) private(i,j) shared(a) num_threads(8)
    {
        mthreadnum = omp_get_num_threads() + 1;
        iam = omp_get_thread_num() + 1;
        isync[iam] = 0;
        #pragma omp barrier

        /* 按列迭代 */
        for (int j = 1; j < N2 - 1; j++) {
            sync_left();
            /* 每列内的行可以并行计算 */
            #pragma omp for schedule(static) nowait
            for (int i = 1; i < N1 - 1; i++) {
                a[i][j] = 0.25 * (a[i-1][j] + a[i][j-1] + a[i+1][j] + a[i][j+1]);
            }
            sync_right();
        }
    }

    end_time = omp_get_wtime();
    used_time = end_time - start_time;

    /* 计算校验和 */
    double sum = 0;
    for (int i = 0; i < N1; i++)
        for (int j = 0; j < N2; j++)
            sum += a[i][j];

    printf("sum=%lf, used_time=%lf seconds\n\n", sum, used_time);
}

int main() {
    init_array();
    jacobi_serial();

    init_array();
    jacobi_column_parallel();

    return 0;
}

/**
 * 数据依赖处理方法：
 *
 * 1. 串行版本：
 *    - 完全串行，无并行
 *    - 作为性能基准
 *
 * 2. 按列并行：
 *    - 列之间有依赖，需要同步
 *    - 列内可以并行计算
 *    - 使用 flush 和自定义同步函数
 *
 * 3. 分块并行：
 *    - 将计算分块，减少同步次数
 *    - 每个块内可以并行计算
 *    - 块之间需要同步
 *
 * 同步机制：
 * 1. #pragma omp flush：刷新缓存，确保数据一致性
 * 2. #pragma omp barrier：屏障同步，等待所有线程
 * 3. 自定义同步：使用共享变量和 flush 实现
 *
 * 性能考虑：
 * 1. 同步开销：频繁同步会降低性能
 * 2. 数据局部性：按列访问可能影响缓存
 * 3. 负载均衡：确保各线程工作量均衡
 */
