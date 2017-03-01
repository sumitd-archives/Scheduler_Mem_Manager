#include "my_pthread_t.h"
#include "my_memory_manager.h"
#include<stdio.h>
#include<stdlib.h>
int global_var = 0;
int threads = 1000;
my_pthread_mutex_t mutex;
void*  function (void * param) {
//void  function (void) {
    //printf("\ninside fucntion \n");
    //while(1) {}
    //int *id = (int*)malloc(sizeof(int));
    //*id = (int)*(running->tcb->id);
    //printf("Inside func for thread %d\n" , GetThreadId());
    int i;
    //int thread = GetThreadId();
    long long current_time = GetCurrentTime();
   // printf("Thread id : %d waiting for mutex\n" , (int)*(running->tcb->id));
    //my_pthread_mutex_lock(&mutex);
    //printf("Thread id : %d got mutex\n" , (int)*(running->tcb->id));
    //printf("Allocating arr for thread = %d\n" , thread);
    printf("Allocating arr\n");// for thread = %d\n" , thread);
    int * arr = (int*)myallocate(500*sizeof(int) , USER);
 //   int * arr = (int*)malloc(500*sizeof(int));
    for (i = 0 ; i < 10 ; i++) {
        arr[i] = 1;
    }
    printf("Allocating arr1\n");// for thread = %d\n" , thread);
    int * arr1 = (int*)myallocate(1000*sizeof(int), USER);
   //    int * arr1 = (int*)malloc(1000*sizeof(int));
   // if (arr1 == NULL) {
   //     printf("Out of memorry for thread id = %d\n" , thread);
   // }
   //printf("Trying to access for thread %d\n" , thread);
   // for (i = 0 ; i < 1000 ; i++)
     //   arr1[i] = thread;
    
    /*for (i = 0 ; i < 10 ; i++) {
        printf("%d , " , arr[i]);
    }*/
 //   while(1){} 
    //while((GetCurrentTime() - current_time) < 20) {
    //sigprocmask(SIG_UNBLOCK , &set , NULL);
   // }
    global_var++;
    int sum = 0;
  //  printf("Trying to access sum for thread %d\n" , thread);
 //   for (i = 0 ; i < 1000 ; i++)
       // sum = sum + arr1[i];

  //  printf("Thread : %d , sum : %d\n" , thread , sum);   
    //printf("Thread id : %d gving mutex\n" , (int)*(running->tcb->id));
 //   my_pthread_mutex_unlock(&mutex);
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

    printf("\nGlobal = %d\n" , global_var);
    printf("Exiting\n");
    end = clock();
    time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    printf("\nTime taken = %lf\n" , time_spent);   
    return 0;
}

