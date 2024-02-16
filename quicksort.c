#include <stdio.h>
#include <string.h>
#include "quicksort.h"

/* Static (private to this file) function prototypes. */
static void swap(void *a, void *b, size_t size);
static int lomuto(void *array, int left, int right, size_t elem_sz,
                  int (*cmp) (const void*, const void*));
static void quicksort_helper(void *array, int left, int right, size_t elem_sz,
                             int (*cmp) (const void*, const void*));

int int_cmp(const void* a, const void* b){
    int* first = (int*) a;
    int* second = (int*) b;
    if(*first > *second){
        return 1;
    }
    else if(*second > *first){
        return -1;
    }

    return 0;
}

int dbl_cmp(const void* a, const void* b){
    double* first = (double*) a;
    double* second = (double*) b;
    if(*first > *second){
        return 1;
    }
    else if(*second > *first){
        return -1;
    }
    return 0;
}

int str_cmp(const void* a, const void* b){
    char** first = (char**) a;
    char** second = (char**) b;

    if(strcmp(*first,*second)>0){
        return 1;
    }
    else if(strcmp(*first,*second)<0){
        return -1;
    }
    return 0;
}

/**
 * Swaps the values in two pointers.
 *
 * Casts the void pointers to type (char *) and works with them as char pointers
 * for the remainder of the function. Swaps one byte at a time, until all 'size'
 * bytes have been swapped. For example, if ints are passed in, size will be 4
 * and this function will swap 4 bytes starting at a and b pointers.
 */
static void swap(void *a, void *b, size_t size) {

    char* first = (char*) a;
    char* second = (char*) b;
    char temp[size];

    for(int i = 0; i < size; i++){
        temp[i] = *first;
        *first++ = *second;
        *second++ = temp[i];
    }
}


/**
 * Partitions array around a pivot, utilizing the swap function. Each time the
 * function runs, the pivot is placed into the correct index of the array in
 * sorted order. All elements less than the pivot should be to its left, and all
 * elements greater than or equal to the pivot should be to its right.
 */


static int lomuto(void *array, int left, int right, size_t elem_sz,
                  int (*cmp) (const void*, const void*)) {
    char* arr = (char*) array;
    char* p = arr+left*elem_sz;
    int s = left;

    for(int i = left + 1; i <= right; i++){
        if(cmp(arr+i*elem_sz ,p) < 0){
            s++;
            swap(arr+s*elem_sz,arr+i*elem_sz,elem_sz);
        }
    }
    swap(arr+left*elem_sz,arr+s*elem_sz,elem_sz);

    return s;
}

/**
 * Sorts with lomuto partitioning, with recursive calls on each side of the
 * pivot.
 * This is the function that does the work, since it takes in both left and
 * right index values.
 */

static void quicksort_helper(void *array, int left, int right, size_t elem_sz,
                             int (*cmp) (const void*, const void*)) {
    if(left < right){
        int pivot = lomuto(array,left,right,elem_sz,cmp);
        quicksort_helper(array,left,pivot-1,elem_sz,cmp);
        quicksort_helper(array,pivot+1,right,elem_sz,cmp);
    }
}

void quicksort(void *array, size_t len, size_t elem_sz,
               int (*cmp) (const void*, const void*)){

    quicksort_helper(array,0,len-1,elem_sz,cmp);

}



/* For testing purposes
int main(int argc,char** argv){
    double array[] = {4.0,21,0.9,3.3,4.2};

    quicksort_helper(array,0,5,sizeof(double*),*dbl_cmp);
    for(int i = 0;i<5;i++){
        printf("%f, ",*(array+i));
    }
}*/




