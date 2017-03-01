#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <time.h>

void print_current_time_with_ms (void)
{
    long            ms; // Milliseconds
    time_t          s;  // Seconds
    struct timespec spec;

    clock_gettime(CLOCK_MONOTONIC, &spec);

    s  = spec.tv_sec;
    ms = round(spec.tv_nsec / 1.0e6); // Convert nanoseconds to milliseconds

    printf("Current time: %"PRIdMAX".%03ld seconds since the Epoch\n",
           (intmax_t)s, ms);
}

int main() {
    print_current_time_with_ms() ;
}
