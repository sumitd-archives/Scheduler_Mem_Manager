#include "my_pthread_t.h"
#include "my_memory_manager.h"
#include<stdio.h>
#include<stdlib.h>
int global_var = 0;
my_pthread_mutex_t mutex;
void*  function (void * param) {
//void  function (void) {
    //printf("\ninside fucntion \n");
    //while(1) {}
    //int *id = (int*)malloc(sizeof(int));
    //*id = (int)*(running->tcb->id);
    int i;
    long long current_time = GetCurrentTime();
   // printf("Thread id : %d waiting for mutex\n" , (int)*(running->tcb->id));
    my_pthread_mutex_lock(&mutex);
    //printf("Thread id : %d got mutex\n" , (int)*(running->tcb->id));
    int * arr = (int*)myallocate(500*sizeof(int) , 1 ,2);
    for (i = 0 ; i < 10 ; i++) {
        arr[i] = GetThreadId();
    }
    for (i = 0 ; i < 10 ; i++) {
        printf("%d , " , arr[i]);
    }
    while((GetCurrentTime() - current_time) < 20) {
    //sigprocmask(SIG_UNBLOCK , &set , NULL);
    }
    int * arr1 = (int*)myallocate(1000*sizeof(int) , 1 ,2);
    if (arr1 == NULL) {
        printf("Out of memorry for thread id = %d\n" , GetThreadId());
    }
    global_var++;
    //printf("Thread id : %d gving mutex\n" , (int)*(running->tcb->id));
    my_pthread_mutex_unlock(&mutex);
    my_pthread_exit(NULL);
    //while(1) {
                //}
}

int main()
{
    clock_t begin, end;
    double time_spent;
    begin = clock();
    my_pthread_scheduler_init();
    my_pthread_mutexattr_t attr;
    my_pthread_mutex_init(&mutex , &attr);
    my_p_thread_t thread[500];
    int i;
    for (i = 0 ; i < 500 ; i++) {
        my_pthread_create(&thread[i] , NULL , &function , NULL);
    }
    for (i = 0 ; i < 500 ; i++) {
        my_pthread_join(thread[i] , NULL);
    }

    printf("\nGlobal = %d\n" , global_var);
    printf("Exiting\n");
    end = clock();
    time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("\nTime taken = %lf\n" , time_spent);   
    return 0;
}

