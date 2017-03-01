#include "my_pthread_t.h"
//#include "my_memory_manager.h"
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/syscall.h>
#include <unistd.h>

#define malloc(x) myallocate(x , USER)
char thread_stack[MEM];
Node * running = NULL;
Node * head = NULL;
Node * tail = NULL;
Node * oldest = NULL;
Waiting * waiting_head = NULL;
Waiting * waiting_tail = NULL;
Completed * completed_head = NULL;
sigset_t set;
int tick_counter = 1;
bool main_exists = false;
ucontext_t main_context;
timer_t timer1 , timer2;
//int global_var = 0;
unsigned unique_id = 1;
void TimerHandlerInit();

void my_pthread_scheduler_init() {
    mem_manager_init();
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
   // sigaddset(&set , SIGRTMAX);
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

int my_pthread_create(my_p_thread_t * thread, my_pthread_attr_t * attr, void* (*function)(void*), void * arg) {
    my_pthread_t * tcb = (my_pthread_t*)myallocate(sizeof(my_pthread_t) , LIBRARY);
    if (tcb == NULL) {
        printf("No more available for library calls . Terminating process\n");
        exit(0);
    }
    ucontext_t * new_thread = (ucontext_t*)myallocate(sizeof(ucontext_t) , LIBRARY);
    if (new_thread == NULL) {
        printf("No more available for library calls . Terminating process\n");
        exit(0);
    }
    getcontext(new_thread);
    new_thread->uc_link = 0;
    //new_thread->uc_stack.ss_sp = malloc(MEM);
    new_thread->uc_stack.ss_sp = thread_stack;
    //new_thread->uc_stack.ss_sp = myallocate(MEM , LIBRARY);
    new_thread->uc_stack.ss_size = MEM;
    new_thread->uc_stack.ss_flags = 0;
    //void (*func) (void) =  (void(*)(void)) function;
    void (*func) (void) =  (void*) function;
    //printf("new_thread = %d\n" , new_thread->uc_stack.ss_size);
    makecontext(new_thread , func , 0);
    sigprocmask(SIG_BLOCK , &set , NULL);
    *thread = unique_id++;
    tcb->id = thread;
    tcb->context = new_thread;
    tcb->burst = DEFAULT_BURST;
    tcb->state = RUNNING;
    tcb->last_start_time = GetCurrentTime();
    tcb->ret_val_ptr = NULL;
    Node * new_node = (Node*)myallocate(sizeof(Node) , LIBRARY);
    if (new_node == NULL) {
        printf("No more available for library calls . Terminating process\n");
        exit(0);
    }
    new_node->tcb = tcb;
    new_node->prev = NULL;
    PushToFront(new_node);
    sigprocmask(SIG_UNBLOCK , &set , NULL);
    return 0;
}

void my_pthread_exit(void * value_ptr) {
    /*disable the scheduler interrupt.
    The scheduler shouldn't be called while the resources 
    are being cleaned up
    */
    sigprocmask(SIG_BLOCK , &set , NULL);

    //Free memory pages
    FreeMemoryPages();
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
    //sigprocmask(SIG_UNBLOCK , &set , NULL);
    
    //call the scheduler
    Scheduler();
}

int my_pthread_join (my_p_thread_t thread , void ** value_ptr) {
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
    return 0;
}

int my_pthread_yield () {
    //Call the scheduler to swap out the current thread
    sigprocmask(SIG_BLOCK , &set , NULL);
    Scheduler();
    return 0;
}

Node * PopulateContext(ucontext_t * context) {
    my_pthread_t * tcb = (my_pthread_t*)myallocate(sizeof(my_pthread_t) , LIBRARY);
    if (tcb == NULL) {
        printf("No more available for library calls . Terminating process\n");
        exit(0);
    }
    my_p_thread_t * thread = (my_p_thread_t*)myallocate(sizeof(my_p_thread_t) , LIBRARY);
    if (thread == NULL) {
        printf("No more available for library calls . Terminating process\n");
        exit(0);
    }
    *thread = unique_id++;
    tcb->id = thread;
    getcontext(context);
    tcb->context = context;
    tcb->burst = DEFAULT_BURST;
    tcb->state = RUNNING;
    tcb->ret_val_ptr = NULL;
    Node * new_node = (Node*)myallocate(sizeof(Node) , LIBRARY);
    if (new_node == NULL) {
        printf("No more available for library calls . Terminating process\n");
        exit(0);
    }
    new_node->tcb = tcb;
    new_node->tcb->last_start_time = GetCurrentTime();
    return new_node;
}

void CompletedListAdd(my_pthread_t * thread) {
    Completed * node = (Completed*)myallocate(sizeof(Completed) , LIBRARY);
    if (node == NULL) {
        printf("No more available for library calls . Terminating process\n");
        exit(0);
    }
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

bool ThreadCompleted(my_p_thread_t thread , void **ptr) {
    Completed * temp = completed_head;
    while (temp != NULL) {
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

void FreeResources(my_p_thread_t thread) {
    Completed * temp = completed_head;
    while ((temp != NULL) && *(temp->next->tcb->id) != thread) {
        temp = temp->next;
    }
    
    if (temp == NULL) {
        return;
    }
    
    //free resources
    Completed * node = temp->next;
    mydeallocate(node->tcb->context->uc_stack.ss_sp , LIBRARY);    
    node->tcb->context->uc_stack.ss_sp = NULL;
    mydeallocate(node->tcb->context , LIBRARY);
    node->tcb->context = NULL;
    mydeallocate(node->tcb , LIBRARY);
    node->tcb = NULL;
    temp->next = node->next;
    node->next = NULL;
    mydeallocate(node , LIBRARY);
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
    }
    else {
        waiting_tail->next = node;
    }
    if (waiting_head == NULL) {
    }
    node->next = NULL;
    waiting_tail = node;
}

Node * GetWaitingHead() {
    if (NULL == waiting_head) {
        return NULL;
    }
    Waiting * temp_head = waiting_head;
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
    int ret = -1;
    while (ret != 1) {
        ret = __sync_lock_test_and_set(mutex , 0);
        if (ret == -1) {
            printf("Mutex not initialized \n");
            __sync_lock_test_and_set(mutex , -1);
            return -1;
        }
        //mutex not available.
        //set the thread to waiting and yield 
        if (ret == 1) {
            break;
        }
        __sync_lock_test_and_set(&running->tcb->state , WAITING);
        my_pthread_yield();
    }
    __sync_lock_test_and_set(&running->tcb->state , RUNNING);
    return 0;
}

int my_pthread_mutex_unlock(my_pthread_mutex_t * mutex) {
    __sync_lock_test_and_set(mutex , 1);
    return 0;        
}

int my_pthread_mutex_destroy(my_pthread_mutex_t * mutex) {
    int ret = 0;
    ret = __sync_lock_test_and_set(mutex , 0);
    if (ret == 0) {
        printf("Error : Couldn't destory mutex because it's locked\n");
        return -1;
    }
    else { 
        ret = __sync_lock_test_and_set(mutex , -1);
    }
    return 0;
}

int my_pthread_mutex_init(my_pthread_mutex_t * mutex , const my_pthread_mutexattr_t * attr) {
    __sync_lock_test_and_set(mutex , 1);
    return 0;
}

void IncreasePriority() {
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

int GetThreadId() {
    Node * current = running;
    if (current->tcb->id == NULL) {
        printf("Address is null\n");
    }
    int thread_id = (int)*(current->tcb->id);
    return thread_id;
}

void Scheduler() {
   if (running != NULL) {
       ProtectContextPages();
       if (tick_counter == 3)  {
       //check for waiting threads
           Node * node = GetWaitingHead();
           if (NULL != node) {
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
       if ((execution_time < (running->tcb->burst)) && (state != WAITING)){
           return;
       }
       else {
           //decrease priority by pushing at end and increase next burst time
           if (state == WAITING) {
               //add to end of waiting queue
               Waiting * node = (Waiting*)myallocate(sizeof(Waiting) , LIBRARY);
	       if (node == NULL) {
	       	   printf("No more available for library calls . Terminating process\n");
	           exit(0);
	       }
               node->node_ptr = running;
               PushToWaitingEnd(node); 
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
           UnprotectContextPages();
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
       UnprotectContextPages();
       sigprocmask(SIG_UNBLOCK , &set , NULL);
       setcontext(running->tcb->context);
   }
}
