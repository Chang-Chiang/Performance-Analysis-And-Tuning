#include<stdio.h>
#include <assert.h>

void bubble_sort(int a[], int n)
{
    int i, j, temp;
    for (j = 0; j < n - 1; j++)
    {
        for (i = 0; i < n - 1 - j; i++)
        {
            if (a[i] > a[i + 1])
            {
                temp = a[i];
                a[i] = a[i + 1];
                a[i + 1] = temp;
            }
        }
    }
}

void insert_sort(int *arr, size_t size)
{
    assert(arr);
    for (int idx = 1; idx <= size - 1; idx++)
    {
        int end = idx;
        int temp = arr[end];
        while (end > 0 && temp < arr[end - 1])
        {
            arr[end] = arr[end - 1];
            end--;
        }
        arr[end] = temp;
    }
}


void select_sort(int a[], int len)
{
    int i, j, temp;
    int minIndex = 0;
    for (i = 0; i < len - 1; i++)
    {
        minIndex = i;

        for (j = i + 1; j < len; j++)
        {
            if (a[j] < a[minIndex])
            {
                minIndex = j;
            }
        }

        if (minIndex != i) {
            temp = a[i];
            a[i] = a[minIndex];
            a[minIndex] = temp;
        }
    }
}

int main() {
    int a[7] = {3, 6, 4, 2, 11, 10, 5};
    bubble_sort(a, 7);
    for (int k = 0; k < 7; k++)
    {
        printf("%d\t", a[k]);
    }
    return 0;
}