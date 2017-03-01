#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<malloc.h>
#include<sys/mman.h>
//#define size ((int)sysconf(_SC_PAGE_SIZE))
char integer[4096];
int main() {
     int page_size = 4096;
     //char * p = (char*)memalign(page_size , 5000);
     //mprotect(p , page_size , PROT_NONE);
     //p[4096] = 'a';
     //printf("Rem = %d \n" , (int)p % 4096);
   /*  int * loc = (int *)integer;
     *loc = 10;
     int * loc1 = (int *) (integer + 4);
     *loc1 = 12;
     FILE * fp = fopen("swap.txt" , "w");
     fwrite(integer , 1 , sizeof(integer) , fp);
     fclose(fp);*/

     int c=0, sum = 0, num = 0;
     int count = 0;
     FILE * fp1 = fopen("swap.txt" , "r");
     while (num < 3) {
     fseek(fp1 , num*4096 , SEEK_SET);
     count = 0;  
     while (count < 4) {
         c = fgetc(fp1);
         *(integer + count) = (char)c;
         ++count;
     }
     printf("val = %d\n" , *(int*)integer);
     ++num;
     }     

 /*    while(count < 4096) {
         c = fgetc(fp1);
         *(integer + count) = (char)c;
         ++count;
     }      
     //printf ("%d , %d \n" , *(integer) , *(integer + 4));
    for (int i = 0 ; i < 600  ; i++) {
        sum = sum + *(int*)(integer + i*4);
    }
    printf("Sum = %d \n" , sum);*/
    return 0;
}
