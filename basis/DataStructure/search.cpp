#include <stdio.h>
#include <malloc.h>
#include <sys/time.h>

// 在数组中 search, 二分查找
int search_arr(int* num, int cnt, int target){
    int first = 0, last = cnt - 1, mid;
        int counter = 0;

        while (first <= last) {
            counter++;
            mid = (first + last) / 2;

            if (num[mid] > target) {
                last = mid - 1;
            }
            else if (num[mid] < target) {
                first = mid + 1;
            }
            else { 
                return 1;
            }
        }
    return 0;
}

// 链表节点定义
struct Node {
    int value;
    struct Node* next;
};

//使用数组来创建一个链表
struct Node* list_create(int data[], int n) {

    //创建头结点
    struct Node* list = (struct Node*)malloc(sizeof(struct Node));

    struct Node* p = list;

    for (int i = 0; i < n; i++) {
        //创建新节点
        struct Node* tmp = (struct Node*)malloc(sizeof(struct Node));

        //设置数据
        tmp->value = data[i];
        p->next = tmp;
        p = p->next;
    }

    p->next = NULL;
    return list;
}

// 在链表中 search
int list_search(struct Node* list, int value) {
    struct Node* p;

    for (p = list->next; p; p = p->next) {
        if (p->value == value) {
            return 1;
        }
    }
    return 0;
}

void list_visit(struct Node* list) {
    for (struct Node* p = list->next; p; p = p->next) {
        printf("%d ", p->value);
    }
}

int main(void) {
    struct timeval start, end;
    int flag = 0;
    int n = 10000000;
    int *num = (int*)malloc(n * sizeof(int));

    for (int i = 0; i < n; i++) {
        num[i] = i;
    }

    gettimeofday(&start, NULL);
    flag = search_arr(num, n, n - 1);
    if (flag) printf("查找成功！");
    else printf("查找成功！");
    gettimeofday(&end, NULL);
    double timeus_arr = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec)/ 1000000.0;
    printf("search in arr, time=%f\n", timeus_arr);

    struct Node* list = list_create(num, n);
    //list_visit(list);
    gettimeofday(&start, NULL);
    flag = list_search(list, n-1);
    if (flag)
        printf("查找成功！");
    else
        printf("查找失败！");
    gettimeofday(&end, NULL);
    double timeuse_list = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
    printf("search in list, time=%f\n", timeuse_list);

    free(num);
    return 0;
}