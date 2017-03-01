#include<stdio.h>
#include<signal.h>
#include<time.h>

timer_t timer1 , timer2;
sigset_t set;

void timerHandler(int signum , siginfo_t *si , void * uc) {
   timer_t * tidp;
   tidp = si->si_value.sival_ptr;

   sigemptyset(&set);
   sigaddset(&set , SIGRTMIN);
   sigprocmask(SIG_UNBLOCK , &set , NULL);
   if ( *tidp == timer1 )
        printf("timer 1\n");
    else if ( *tidp == timer2 ) {
        printf("timer 2\n");
        }
   while(1) {}
}

void handlerInit() {
   struct sigaction sa;
   int signum = SIGRTMIN;
   sa.sa_flags = SA_SIGINFO;
   sa.sa_sigaction = timerHandler;
   sigemptyset(&sa.sa_mask);
   if (sigaction(signum , &sa , NULL) == -1) {
      perror("error in registering handler\n");
   }
}

void createTimer(timer_t * timerId , int interval) {
    struct sigevent event_init;
    event_init.sigev_notify = SIGEV_SIGNAL;
    int signum = SIGRTMIN;
    event_init.sigev_signo = signum;
    event_init.sigev_value.sival_ptr = timerId;
    timer_create(CLOCK_REALTIME , & event_init , timerId);

    struct itimerspec its;
    its.it_interval.tv_sec = interval;
    its.it_interval.tv_nsec = 0;  
    its.it_value.tv_sec = 0;
    its.it_value.tv_nsec = interval;
    timer_settime(*timerId , 0 , &its , NULL);

}

int main() {
    handlerInit();
    createTimer(&timer1 , 1);
    createTimer(&timer2 , 5);
    
    while(1){}
}       


