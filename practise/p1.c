#include<stdio.h>
int main()
{
    int a = 10;
    int *b = &a;
    int c = (int)b;
    printf("integer = %x\n" , c);
    printf("address = %p \n" , b);

    void * d = (void *)c;
    printf("value of d  = %p \n" , d);
    if (c == (int)b) {
    printf("They are equal \n");
    }
}    

