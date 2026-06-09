// 依赖分析 (Dependency Analysis)
//
// 什么是依赖分析？
//   分析程序中语句之间的依赖关系，确定哪些操作可以并行执行。
//
// 依赖类型：
//   1. 真依赖 (RAW, Read After Write) — 读取之前写入的数据
//   2. 反依赖 (WAR, Write After Read) — 写入之前读取的数据
//   3. 输出依赖 (WAW, Write After Write) — 写入之前写入的数据
//
// 本例分析：
//   for (i = 1; i < N; i++) {
//       if (a[i] > x) {  // S1：读取 x
//           x = a[i];    // S2：写入 x
//       }
//   }
//
//   S1 和 S2 之间存在循环携带依赖：
//     - S1 读取 x（来自上一次迭代的 S2 或初始值）
//     - S2 写入 x（供下一次迭代的 S1 使用）
//     - 这是真依赖 (RAW)，无法并行化
//
// 编译命令：
//   g++ -O2 dependency.cpp -o dependency

#include <stdio.h>

int main() {
    int a[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    int x = a[0];
    int N = sizeof(a) / sizeof(int);

    // 循环携带依赖：x 在迭代之间传递
    for (int i = 1; i < N; i++) {
        if (a[i] > x) {  // S1：读取 x（真依赖 RAW）
            x = a[i];    // S2：写入 x
        }
    }

    printf("Max value: x = %d\n", x);
    return 0;
}
