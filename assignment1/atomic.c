#include <stdio.h>
#include <sys/syscall.h>

int main() {
    int i = 1;
    int ret = __sync_lock_test_and_set(&i, 0);

    printf("ret = %d" , ret);
    return 0;
}
