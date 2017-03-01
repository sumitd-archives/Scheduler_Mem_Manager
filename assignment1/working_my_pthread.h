#include <pthread.h>
#include <ucontext.h>
#include <stdbool.h>

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
//void my_pthread_create(pthread_t * , pthread_attr_t * , void* (*function)(void*) , void *);
void my_pthread_create(my_p_thread_t * , my_pthread_attr_t * , void (*function)(void) , void *);
void my_pthread_exit();
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
void function(void);

#define DEFAULT_BURST 2
unsigned unique_id = 1;
#define MEM 64000
