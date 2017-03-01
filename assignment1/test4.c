#include<stdio.h>
#include<stdlib.h>
#include<ucontext.h>
#define MEM 64000

ucontext_t T1;

void handler(int signum) {
    setcontext(&T1);
}

void func() {
    printf("function for thread \n");
}

void CreateThread() {
   getcontext(&T1);
   T1.uc_link = 0;
   T1.uc_stack.ss_sp = malloc(MEM);
   T1.uc_stack.ss_size = MEM;
   T1.uc_stack.ss_flags = 0;
   makecontext(&T1 , (void*)&func , 0);
}

int main() {
    CreateThread();
    signal(SIGINT , handler);
    while(1) {}
}     

