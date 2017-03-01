#include "my_pthread_t.h"
#include<stdio.h>
#include<stdlib.h>
int threads = 100;
my_pthread_mutex_t mutex;
void*  function (void * param) {
    int i, count;
    int thread_id = GetThreadId();
    //allocating array for 500 integers
    printf("Allocating memory for 500 integers for thread id = %d\n" , thread_id);
    count = 500;
    int * arr = (int*)malloc(count*sizeof(int));
    if (arr == NULL) {
        printf("No more memory left to be allocated for threads . Exiting process\n");
        exit(0);
    }
    int sum = 0;
    for (i = 0 ; i < count ; i++) {
        arr[i] = thread_id;
        sum = sum + arr[i];
    }

    //allocating array for 1000 more integers .
    //This will lead to allocation of a new page for this thread because
    //1500 integers wont fit in a single page
    count = 1000;
    printf("Allocating memory for 1000 more integers for thread id = %d\n" , thread_id);
    int * arr1 = (int*)malloc(count*sizeof(int));
    if (arr1 == NULL) {
        printf("No more memory left to be allocated for threads . Exiting process\n");
        exit(0);
    }
    int sum1 = 0;
    for (i = 0 ; i < count ; i++) {
        arr1[i] = thread_id;
        sum1 = sum1 + arr1[i];
    }

   printf("Sum for thread_id : %d =   thread_id*500 =  %d\n" , thread_id , sum);
   printf("Sum1 for thread_id : %d =   thread_id*500 =  %d\n" , thread_id , sum1);

   my_pthread_exit(NULL);
}

int main()
{
    clock_t begin, end;
    double time_spent;
    begin = clock();
    my_pthread_scheduler_init();
    my_pthread_mutexattr_t attr;
    my_pthread_mutex_init(&mutex , &attr);
    my_p_thread_t thread[threads];
    int i;
    for (i = 0 ; i < threads ; i++) {
        my_pthread_create(&thread[i] , NULL , &function , NULL);
    }
    for (i = 0 ; i < threads ; i++) {
        my_pthread_join(thread[i] , NULL);
    }

    printf("Exiting\n");
    end = clock();
    time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("\nTime taken = %lf\n" , time_spent);   
    return 0;
}

