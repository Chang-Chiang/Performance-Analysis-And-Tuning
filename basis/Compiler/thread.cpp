// 多线程编程示例 (Multi-threading)
//
// 什么是多线程？
//   多线程是程序同时执行多个任务的能力。
//   每个线程是程序中的一个独立执行路径，共享进程的内存空间。
//
// C++11 多线程的作用:
//   1. 并行计算 — 利用多核 CPU 同时执行多个任务
//   2. 提升性能 — 将大任务拆分为小任务并行执行
//   3. 异步执行 — 主线程继续执行，子线程在后台处理任务
//
// std::thread 的基本用法:
//   thread myobj(func)  — 创建线程，执行 func 函数
//   myobj.join()        — 等待线程结束
//   myobj.detach()      — 分离线程（后台运行）
//
// 编译命令:
//   clang++ -std=c++11 thread.cpp -o thread -lpthread
//
// 选项:
//   -std=c++11  启用 C++11 标准
//   -lpthread   链接 POSIX 线程库

#include <iostream>
#include <thread>

using namespace std;

void myprint() {
    cout << "线程开始执行" << endl;
    // 子线程执行的任务
    cout << "线程执行结束" << endl;
}

int main() {
    thread myobj(myprint);  // 创建子线程
    myobj.join();           // 等待子线程结束
    cout << "主线程执行" << endl;
    return 0;
}
