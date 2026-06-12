/**
 * 间接内存访问优化示例
 *
 * 原理：
 * 间接内存访问是指通过指针或索引数组来访问数据。
 * 这种访问模式通常会导致缓存未命中，因为访问地址难以预测。
 *
 * 本例展示了两种间接访问模式：
 * 1. 数组的间接访问：通过索引数组访问数据
 * 2. 指针链表：通过指针遍历数据
 *
 * 编译指令：
 * gcc -O2 -o indirect_memory_access indirect_memory_access.c
 *
 * 运行：
 * ./indirect_memory_access
 */

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

/* 片段 1：数组的间接访问 */
void indirect_array_access() {
    #define NUM_KEYS 8

    int i, key;
    int key_array[NUM_KEYS];
    int bucket_ptrs[NUM_KEYS];
    int key_buff2[NUM_KEYS];
    int shift = 1;

    /* 初始化数组 */
    for (i = 0; i < NUM_KEYS; i++) {
        key_array[i] = i;
        bucket_ptrs[i] = i;
        key_buff2[i] = 0;
    }

    /* 间接访问示例
     * 问题：bucket_ptrs[key >> shift] 的访问地址难以预测
     * 导致缓存预取器无法有效工作 */
    for (i = 0; i < NUM_KEYS; i++) {
        key = key_array[i];
        key_buff2[bucket_ptrs[key >> shift]++] = key;
    }

    printf("数组间接访问结果：\n");
    for (i = 0; i < NUM_KEYS; i++) {
        printf("%d  ", key_buff2[i]);
    }
    printf("\n");
}

/* 片段 2：指针链表 */
typedef struct ListNode {
    int Element;
    struct ListNode* next;
} Node, *PNode;

/* 初始化链表 */
Node* initLink() {
    Node* p = (Node*)malloc(sizeof(Node));  /* 创建头结点 */
    Node* temp = p;  /* 遍历指针 */

    for (int i = 1; i < 10; i++) {
        Node* a = (Node*)malloc(sizeof(Node));
        a->Element = i;
        a->next = NULL;
        temp->next = a;
        temp = temp->next;
    }
    return p;
}

/* 显示链表 */
void display(Node* p) {
    Node* temp = p;
    while (temp->next) {
        temp = temp->next;
        printf("%d  ", temp->Element);
    }
    printf("\n");
}

/* 查找元素 */
int selectElem(Node* p, int M) {
    Node* temp;
    for (; p->next; p = p->next) {
        if (p->Element == M) {
            temp = p;
            return 1;
        }
    }
    return -1;
}

/* 链表访问示例 */
void linked_list_access() {
    printf("初始化链表为：\n");
    Node* p = initLink();
    display(p);

    int temp = selectElem(p, 5);
    if (temp == 1)
        printf("已查找到元素 5\n");
    else
        printf("查找失败\n");

    /* 释放链表内存 */
    Node* current = p;
    while (current) {
        Node* next = current->next;
        free(current);
        current = next;
    }
}

int main() {
    printf("=== 数组间接访问 ===\n");
    indirect_array_access();

    printf("\n=== 指针链表访问 ===\n");
    linked_list_access();

    return 0;
}

/**
 * 间接访问的性能问题：
 * 1. 访存地址难以预测，缓存预取器无法有效工作
 * 2. 每次访问都可能产生缓存未命中
 * 3. 指针追逐（Pointer Chasing）会导致严重的性能下降
 *
 * 优化建议：
 * 1. 尽量使用直接访问模式（连续内存访问）
 * 2. 如果必须使用间接访问，可以考虑：
 *    - 预取数据：使用 __builtin_prefetch 提前加载数据
 *    - 数据局部化：将相关数据放在连续内存中
 *    - 使用数组代替链表：数组的缓存友好性更好
 * 3. 对于哈希表等数据结构，考虑使用开放寻址法代替链地址法
 */
