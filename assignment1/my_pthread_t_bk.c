#include "my_pthread_t.h"
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/syscall.h>
#include <unistd.h>

my_pthread_mutex_t mutex = true;
Node * running = NULL;
Node * head = NULL;
Node * tail = NULL;
Node * oldest = NULL;
Waiting * waiting_head = NULL;
Waiting * waiting_tail = NULL;
Completed * completed_head = NULL;
sigset_t set;
sigset_t set1;
int tick_counter = 1;
bool main_exists = false;
ucontext_t main_context;
timer_t timer1 , timer2;
int global_var = 0;
void TimerHandlerInit();

void CreateThreads(int signum) {
    sigaddset(&set1 , SIGINT);
    sigprocmask(SIG_BLOCK , &set1 , NULL);
    pthread_t th;
    my_pthread_create(&th , NULL , &function , NULL);
    my_pthread_join(th , NULL);
}

void my_pthread_scheduler_init() {
    TimerHandlerInit();
    if (!main_exists) {
      //This is the first time the scheduler has been invoked.
      //Add the main context to the queue such that the main thread can 
      //scheduled as well
      Node * node = PopulateContext(&main_context);
      running = node;
      main_exists = true;
   }
   if(NULL == oldest) {
      oldest = running;
   }
   sigemptyset(&set);
   sigaddset(&set , SIGRTMIN);
   createTimer(&timer1 , 50 , SIGRTMIN); 
   createTimer(&timer2 , 100 , SIGRTMIN); 
}
 
void timerHandler(int signum , siginfo_t *si , void * uc) {
   timer_t * tidp;
   tidp = si->si_value.sival_ptr;
   if ( *tidp == timer1 ) {
        //call the scheduler
        Scheduler();
   }
   else if ( *tidp == timer2 ) {
        //give highest priority to oldest ;
       IncreasePriority();
   }
}

void TimerHandlerInit() {
   struct sigaction sa;
   int signum = SIGRTMIN;
   sa.sa_flags = SA_SIGINFO;
   sa.sa_sigaction = timerHandler;
   sigemptyset(&sa.sa_mask);
   if (sigaction(signum , &sa , NULL) == -1) {
      perror("error in registering handler\n");
   }
}

void createTimer(timer_t * timerId , int interval , int signum) {
    struct sigevent event_init;
    event_init.sigev_notify = SIGEV_SIGNAL;
    //int signum = SIGRTMIN;
    event_init.sigev_signo = signum;
    event_init.sigev_value.sival_ptr = timerId;
    timer_create(CLOCK_REALTIME , & event_init , timerId);

    struct itimerspec its;
    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = interval*1000000;
    its.it_value.tv_sec = 0;
    its.it_value.tv_nsec = interval*1000000;
    //its.it_value.tv_nsec = 0;
    timer_settime(*timerId , 0 , &its , NULL);
}

//void*  function (void * param) {
void  function (void) {
    //printf("\ninside fucntion \n");
    //while(1) {}
    int *id = (int*)malloc(sizeof(int));
    *id = (int)*(running->tcb->id);
    long long current_time = GetCurrentTime();
    //printf("Thread id : %d waiting for mutex\n" , (int)*(running->tcb->id));
    //my_pthread_mutex_lock(&mutex);
    //printf("Thread id : %d got mutex\n" , (int)*(running->tcb->id));
    while((GetCurrentTime() - current_time) < 500) {
    //sigprocmask(SIG_UNBLOCK , &set , NULL);
    }
    global_var++;
    //printf("Thread id : %d gving mutex\n" , (int)*(running->tcb->id));
    //my_pthread_mutex_unlock(&mutex);
    my_pthread_exit(id);
    //while(1) {
                //}
}

void function1 (void) {
    my_pthread_mutex_lock(&mutex);
    my_pthread_mutex_unlock(&mutex);
    my_pthread_exit(NULL);
}

void my_pthread_create(pthread_t * thread, pthread_attr_t * attr, void (*function)(void), void * arg) {
    my_pthread_t * tcb = (my_pthread_t*)malloc(sizeof(my_pthread_t));
    ucontext_t * new_thread = (ucontext_t*)malloc(sizeof(ucontext_t));
    getcontext(new_thread);
    //Mutex required
    new_thread->uc_link = 0;
    new_thread->uc_stack.ss_sp = malloc(MEM);
    new_thread->uc_stack.ss_size = MEM;
    new_thread->uc_stack.ss_flags = 0;
    //void (*func) (void) =  (void(*)(void)) function;
    void (*func) (void) =  (void*) function;
    makecontext(new_thread , function , 0);
    sigprocmask(SIG_BLOCK , &set , NULL);
    *thread = unique_id++;
    tcb->id = thread;
    tcb->context = new_thread;
    tcb->burst = DEFAULT_BURST;
    tcb->state = RUNNING;
    tcb->last_start_time = GetCurrentTime();
    tcb->ret_val_ptr = NULL;
    Node * new_node = (Node*)malloc(sizeof(Node));
    new_node->tcb = tcb;
    new_node->prev = NULL;
    PushToFront(new_node);
    sigprocmask(SIG_UNBLOCK , &set , NULL);
}

void my_pthread_exit(void * value_ptr) {
    /*disable the scheduler interrupt.
    The scheduler shouldn't be called while the resources 
    are being cleaned up
    */
    sigprocmask(SIG_BLOCK , &set , NULL);

    /*Change the state to terminated and add this thread to 
    the list of completed threads*/
    running->tcb->state = TERMINATED;
    running->tcb->ret_val_ptr = value_ptr;
    CompletedListAdd(running->tcb);

    /*Invalidate this thread.
    Shouldn't be scheduled again*/
    running->next = NULL;
    running->prev = NULL;
    running = NULL;
    sigprocmask(SIG_UNBLOCK , &set , NULL);
    
    //call the scheduler
    Scheduler();
}

void my_pthread_join (pthread_t thread , void ** value_ptr) {
    //Check if thread present in completed list and get the return value
    //else wait till the thread completed
    sigprocmask(SIG_BLOCK , &set , NULL);
    while(!ThreadCompleted(thread , value_ptr)) 
    {
       // my_pthread_yield();
        sigprocmask(SIG_UNBLOCK , &set , NULL);
        my_pthread_yield();
    }
    
    //free up the terminated thread's resources
    //FreeResources(thread);
    sigprocmask(SIG_UNBLOCK , &set , NULL);
}

int my_pthread_yield () {
    //Call the scheduler to swap out the current thread
    sigprocmask(SIG_BLOCK , &set , NULL);
    Scheduler();
    return 0;
}

Node * PopulateContext(ucontext_t * context) {
    my_pthread_t * tcb = (my_pthread_t*)malloc(sizeof(my_pthread_t));
    pthread_t * thread = (pthread_t*)malloc(sizeof(pthread_t));
    *thread = unique_id++;
    tcb->id = thread;
    getcontext(context);
    tcb->context = context;
    tcb->burst = DEFAULT_BURST;
    tcb->state = RUNNING;
    tcb->ret_val_ptr = NULL;
    Node * new_node = (Node*)malloc(sizeof(Node));
    new_node->tcb = tcb;
    new_node->tcb->last_start_time = GetCurrentTime();
    return new_node;
}

void CompletedListAdd(my_pthread_t * thread) {
    Completed * node = (Completed*)malloc(sizeof(Completed));
    node->tcb = thread;
    node->next = NULL;
    //sigprocmask(SIG_BLOCK , &set , NULL);
    if (completed_head == NULL) {
        completed_head = node;
        return;
    }
    //Add at the end
    Completed * temp = completed_head;
    while (temp->next != NULL) {
        temp=temp->next;  
    }
    temp->next = node;
  //  sigprocmask(SIG_UNBLOCK , &set , NULL);
}

bool ThreadCompleted(pthread_t thread , void **ptr) {
    Completed * temp = completed_head;
    while (temp != NULL) {
           // printf("Line 173 \n");
        if (*(temp->tcb->id) == thread) {
            if (ptr != NULL) {
                *ptr = temp->tcb->ret_val_ptr;
            }
            return true;
        }
        temp = temp->next;
    }
    return false;  
}

void FreeResources(pthread_t thread) {
    Completed * temp = completed_head;
    while ((temp != NULL) && *(temp->next->tcb->id) != thread) {
        temp = temp->next;
    }
    
    if (temp == NULL) {
        return;
    }
    
    //free resources
    Completed * node = temp->next;
    free(node->tcb->context->uc_stack.ss_sp);    
    node->tcb->context->uc_stack.ss_sp = NULL;
    free(node->tcb->context);
    node->tcb->context = NULL;
    free(node->tcb);
    node->tcb = NULL;
    temp->next = node->next;
    node->next = NULL;
    free(node);
    node = NULL;
}

void PushToFront(Node * node) {
    //create new tcb node
    if (NULL == head) {
        node->next == NULL;
        tail = node;  
    }
    else {
        node->next = head;
        head->prev = node;
    }    
    head = node;
    head->prev = NULL;
}

void PushToEnd(Node * node) {
    if (NULL == head) {
        node->prev = NULL;
        head = node;
    }
    else {
       tail->next = node;
       node->prev = tail;
    }
    node->next = NULL;
    tail = node;
}

Node * GetHead() {
    if (head == NULL) {
        return NULL;
    }
    Node * temp = head;
    if (head->next == NULL) {
        head = NULL;
    }
    else {
        head = head->next;
        head->prev = NULL;
    }
    return temp;    
}

void PushToWaitingEnd(Waiting * node) {
    if (NULL == waiting_head) {
        waiting_head = node;
        //printf("Adding waiting head\n");
    }
    else {
        waiting_tail->next = node;
    }
    if (waiting_head == NULL) {
        //printf("Waiting head is null\n");
    }
    node->next = NULL;
    waiting_tail = node;
}

Node * GetWaitingHead() {
    if (NULL == waiting_head) {
        return NULL;
    }
    //printf("Returning waiting head \n");
    Waiting * temp_head = waiting_head;
    //free(waiting_head);
    waiting_head = temp_head->next;
    return temp_head->node_ptr;   
}

long long GetCurrentTime() {
    long ms;
    long sec;    
    long long time_ms;  
    struct timespec spec;
    clock_gettime(CLOCK_MONOTONIC , &spec);
    sec = spec.tv_sec;
    ms = spec.tv_sec;
    time_ms = sec * 1000 + ms;
    return time_ms;
}

int my_pthread_mutex_lock(my_pthread_mutex_t * mutex) {
    int ret;
    while((ret = __sync_lock_test_and_set(mutex , 0)) == 0) {
        //mutex not available.
        //set the thread to waiting and yield 
        __sync_lock_test_and_set(&running->tcb->state , WAITING);
        my_pthread_yield();
    }
    __sync_lock_test_and_set(&running->tcb->state , RUNNING);
   //printf("Changed to running for thread id : %d \n" , (int)*(running->tcb->id));
    return 0;
}

int my_pthread_mutex_unlock(my_pthread_mutex_t * mutex) {
    __sync_lock_test_and_set(mutex , 1);
    return 0;        
}

void IncreasePriority() {
   // sigprocmask(SIG_BLOCK , &set , NULL);
    if (oldest == running) {
        return;
    }

    long long prev = 0;
    long long next = 0;
    long long current = GetCurrentTime() - oldest->tcb->last_start_time;
    if ((oldest->prev == NULL) && (oldest->next == NULL)) {
        return;
    }
   
    if (oldest->prev != NULL) {
        prev = GetCurrentTime() - oldest->prev->tcb->last_start_time;
    }
    if (oldest->next != NULL) {
        next = GetCurrentTime() - oldest->next->tcb->last_start_time;
    }
    Node * temp = oldest;
    if ((temp->prev != NULL) && (temp->next != NULL)) {
        temp->prev->next = temp->next;
        temp->next->prev = temp->prev;
    }
    else if(temp->prev == NULL) {
        temp->next->prev = NULL;
    }
    else {
        temp->prev->next = NULL;
    }
    printf("%llu , %llu , %llu , %d\n" , prev , next , current , (int)*(temp->tcb->id));
    if (prev > next) {
        //prev should be set as the next oldest
        oldest = temp->prev;
    }
    else {
        oldest = temp->next;
    }
    //set temp as the head;
    PushToFront(temp);
    //sigprocmask(SIG_UNBLOCK , &set , NULL);
}

void Scheduler() {
   //printf("Inside scheduler\n");
   if (running != NULL) {
       if (tick_counter == 3)  {
       //check for waiting threads
           Node * node = GetWaitingHead();
           if (NULL != node) {
               printf("Got threadid : %d from wait\n" , (int) *(node->tcb->id));
               PushToFront(node);    
           }
           tick_counter = 0;
       }
       else {
           tick_counter++;
           if (head == NULL) {
               return;
           }
       }
       //check if the running thread has executed for complete time slice
       states state = running->tcb->state;
       long long execution_time = GetCurrentTime() -  running->tcb->last_start_time;
       if ((execution_time < (running->tcb->burst)) && (state != WAITING)) {
           return;
       }
       else {
           //decrease priority by pushing at end and increase next burst time
           if (state == WAITING) {
               //add to end of waiting queue
               Waiting * node = (Waiting*)malloc(sizeof(Waiting));
               node->node_ptr = running;
               PushToWaitingEnd(node); 
               printf("Push to waiting end thread id = %d \n" , (int)*(running->tcb->id));
           }
           else {
               //push to end of running queue
               running->tcb->last_start_time = GetCurrentTime();
               running->tcb->burst +=  DEFAULT_BURST;
               PushToEnd(running);
           }
           //get the head
           Node * current = running;
           running = GetHead(); 
           if (NULL == running) {
               return;
           }
           if (current == oldest) {
               //set the next node in the queue as the oldest
               oldest = running;
           }
           printf("Starting thread id = %d \n" , (int)*(running->tcb->id));
           sigprocmask(SIG_UNBLOCK , &set , NULL);
           swapcontext(current->tcb->context , running->tcb->context);
       }
   }
   else if (NULL == head) {
       //nothing to schedule
       return;
   }
   else {
       running = GetHead();
       printf("Starting thread id = %d \n" , (int)*(running->tcb->id));
       sigprocmask(SIG_UNBLOCK , &set , NULL);
       setcontext(running->tcb->context);
   }
}
          
int main()
{
    signal(SIGINT , CreateThreads);
    my_pthread_scheduler_init();
    pthread_t thread[50];
    int i;
    for (i = 0 ; i < 50 ; i++) {
        my_pthread_create(&thread[i] , NULL , &function , NULL);
    }
    for (i = 0 ; i < 50 ; i++) {
        my_pthread_join(thread[i] , NULL);
    }

    printf("\nGlobal = %d\n" , global_var);
    printf("Exiting\n");
    return 0;
}
