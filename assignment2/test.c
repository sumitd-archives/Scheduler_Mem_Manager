#include<stdio.h>
int main() {
    int c =0;
    int ch;
    char num[4];
    FILE * fp = fopen("log.txt" , "r");
    while (c < 4) {
        ch = fgetc(fp);
        *(num + c) = (char)ch;
        c++;
    }
    printf("number = %p\n", num);
    return 0;
}
