#include<stdio.h>
#include<stdlib.h>
#include<ucontext.h>
#define MEM 64000

ucontext_t T1 , T2;

int running_thread = 0;

void synchronize() {
    if (running_thread == 0) { //no thread running run the first one
        running_thread = 1; 
        setcontext(&T1);
    }
    else if (running_thread == 1) {
        running_thread = 2;
        swapcontext(&T1 , &T2);
    }
    else if (running_thread == 2) {
        running_thread = 1;
        swapcontext(&T2 , &T1);
    }
}

int func() {
    printf ("Thread : %d - Line 1\n" , running_thread);
    synchronize();
    printf ("Thread : %d - Line 2\n" , running_thread);
    synchronize();
    printf ("Thread : %d - Line 3\n" , running_thread);
    synchronize();
    printf ("Thread : %d - Line 4\n" , running_thread);
    synchronize();
    printf ("Thread : %d - Line 5\n" , running_thread);
    synchronize();
    printf ("Thread : %d - Line 6\n" , running_thread);
    synchronize();
    printf ("Thread : %d - Line 7\n" , running_thread);
    synchronize();
    printf ("Thread : %d - Line 8\n" , running_thread);
    synchronize();
    printf ("Thread : %d - Line 9\n" , running_thread);
    synchronize();
    printf ("Thread : %d - Line 10\n" , running_thread);
    synchronize();
}

void CreateThread(ucontext_t * thread) {
    getcontext(thread);
    thread->uc_link = 0;
    thread->uc_stack.ss_sp = malloc(MEM);
    thread->uc_stack.ss_size = MEM;
    thread->uc_stack.ss_flags = 0;
    makecontext(thread , (void*)&func , 0);
}

int main() {
    //Create two threads;
    CreateThread(&T1);
    CreateThread(&T2);
    //synchronize();
    printf("Main thread Exiting \n");
}
