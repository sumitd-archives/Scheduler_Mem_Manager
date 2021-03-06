#ifndef MY_PTHREAD_H
#define MY_PTHREAD_H

#include <ucontext.h>
#include <stdbool.h>
#include <time.h>
#include "my_memory_manager.h"
typedef enum states
{
    RUNNING,
    TERMINATED,
    WAITING
}states;

typedef int my_pthread_mutex_t;
typedef int my_p_thread_t;
typedef int my_pthread_mutexattr_t;
typedef int my_pthread_attr_t;
 
typedef struct my_pthread_t{
    my_p_thread_t * id;
    ucontext_t * context;
    int burst;
    long long last_start_time;
    states state;
    void* ret_val_ptr; 
} my_pthread_t;

typedef struct Node{
    my_pthread_t * tcb;
    struct Node * prev;
    struct Node * next;
} Node;

typedef struct Completed {
    my_pthread_t * tcb;
    struct Completed * next;
} Completed;

typedef struct Waiting {
    Node * node_ptr;
    struct Waiting * next;
} Waiting;

void CompletedListAdd(my_pthread_t* thread);
bool ThreadCompleted(my_p_thread_t , void**);
void FreeResources(my_p_thread_t);
int  my_pthread_create(my_p_thread_t * , my_pthread_attr_t * , void* (*function)(void*) , void *);
//void my_pthread_create(my_p_thread_t * , my_pthread_attr_t * , void (*function)(void) , void *);
int my_pthread_join(my_p_thread_t , void **);
void my_pthread_exit(void *);
int my_pthread_yield();
int my_pthread_mutex_lock(my_pthread_mutex_t * mutex);
int my_pthread_mutex_unlock(my_pthread_mutex_t * mutex);
int my_pthread_mutex_destroy(my_pthread_mutex_t * );
int my_pthread_mutex_init(my_pthread_mutex_t * , const my_pthread_mutexattr_t *); 
Node * PolpulateContext(ucontext_t*);
void PushToFront(Node*);
void PushToEnd(Node*);
long long GetCurrentTime();
Node * GetHead();
void IncreasePriority();
Node * PopulateContext(ucontext_t *);
void Scheduler();
void my_pthread_scheduler_init();
void createTimer(timer_t * , int , int);
int GetThreadId(); 
//void function(void);

#define DEFAULT_BURST 2
#define MEM 6144 
#endif
