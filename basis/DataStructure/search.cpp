// 数据结构对性能的影响 (Impact of Data Structures on Performance)
//
// 本例比较不同数据结构（数组 vs 链表）对查找性能的影响。
//
// 数据结构比较：
//   数据结构   内存布局   缓存友好性   随机访问   插入/删除
//   数组       连续内存   高           O(1)       O(n)
//   链表       分散内存   低           O(n)       O(1)
//
// 查找算法比较：
//   算法         数据结构   时间复杂度   缓存命中率
//   二分查找     有序数组   O(log n)     高（连续内存，预取友好）
//   顺序查找     链表       O(n)         低（内存分散，缓存不友好）
//
// 性能差异原因：
//   1. 数组连续存储，CPU 缓存预取效率高
//   2. 链表节点分散在堆中，每次访问可能触发缓存未命中
//   3. 二分查找减少比较次数，顺序查找需要逐个遍历
//
// 编译命令：
//   clang++ -O1 search.cpp -o search

#include <malloc.h>
#include <stdio.h>
#include <sys/time.h>

// 二分查找：在有序数组中查找目标值
// 时间复杂度：O(log n)
int search_arr(int* num, int cnt, int target) {
    int first = 0, last = cnt - 1, mid;

    while (first <= last) {
        mid = (first + last) / 2;

        if (num[mid] > target) {
            last = mid - 1;
        } else if (num[mid] < target) {
            first = mid + 1;
        } else {
            return 1;  // 查找成功
        }
    }
    return 0;  // 查找失败
}

// 链表节点定义
struct Node {
    int value;
    struct Node* next;
};

// 使用数组创建链表
struct Node* list_create(int data[], int n) {
    // 创建头结点
    struct Node* list = (struct Node*)malloc(sizeof(struct Node));
    struct Node* p = list;

    for (int i = 0; i < n; i++) {
        // 创建新节点
        struct Node* tmp = (struct Node*)malloc(sizeof(struct Node));
        tmp->value = data[i];
        p->next = tmp;
        p = p->next;
    }

    p->next = NULL;
    return list;
}

// 顺序查找：在链表中查找目标值
// 时间复杂度：O(n)
int list_search(struct Node* list, int value) {
    for (struct Node* p = list->next; p; p = p->next) {
        if (p->value == value) {
            return 1;  // 查找成功
        }
    }
    return 0;  // 查找失败
}

// 遍历链表
void list_visit(struct Node* list) {
    for (struct Node* p = list->next; p; p = p->next) {
        printf("%d ", p->value);
    }
    printf("\n");
}

// 释放链表内存
void list_free(struct Node* list) {
    struct Node* p = list;
    while (p) {
        struct Node* tmp = p;
        p = p->next;
        free(tmp);
    }
}

int main(void) {
    struct timeval start, end;
    int n = 10000000;
    int* num = (int*)malloc(n * sizeof(int));

    // 初始化有序数组
    for (int i = 0; i < n; i++) {
        num[i] = i;
    }

    // 测试数组二分查找
    gettimeofday(&start, NULL);
    int flag = search_arr(num, n, n - 1);
    gettimeofday(&end, NULL);
    double time_arr = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
    printf("Binary Search (Array):  %s, time=%.6f s\n", flag ? "Found" : "Not Found", time_arr);

    // 创建链表
    struct Node* list = list_create(num, n);

    // 测试链表顺序查找
    gettimeofday(&start, NULL);
    flag = list_search(list, n - 1);
    gettimeofday(&end, NULL);
    double time_list = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
    printf("Sequential Search (List): %s, time=%.6f s\n", flag ? "Found" : "Not Found", time_list);

    // 释放内存
    free(num);
    list_free(list);

    return 0;
}
