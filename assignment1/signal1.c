#include <stdio.h>
#include <signal.h>
#include <unistd.h>

void signal_handle(int signal)
{
    printf("Wont kill\n");
}

int main() {
    signal(SIGINT , signal_handle);
    while (1) {
    }
}
