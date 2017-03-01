#include<stdio.h>
#include "my_memory_manager.h"
#include "my_pthread_t.h"
typedef struct book_keep
{
    int thread;
    int last_offset;
}book_keep;

#define mem_size 8*1024*1024
#define mem_pages 1788
#define book_keeping_pages 4
#define library_mem 1024*1024
#define page_size 4*1024
char *book_keeping_start_loc ;

void display(char * mem) {
    char * book_keeping_current_loc = book_keeping_start_loc;
    int size_int = sizeof(int);
    for (int i = 0 ; i < 10 ; i++) {
        printf ("%d " ,(int)*book_keeping_current_loc);
        book_keeping_current_loc += size_int;
        printf ("%d \n" ,(int)*book_keeping_current_loc);
        book_keeping_current_loc += size_int;
    }
}

char * GetMemoryLoc() {
    static char mem[mem_size] ;
    return mem;
}
    
int mem_manager_init() {
    char * mem = GetMemoryLoc();
    int size_int = sizeof(int);
    book_keeping_start_loc = mem + library_mem;
    int half_size = sizeof(book_keep)/2;
    char *book_keeping_current_loc = book_keeping_start_loc; 
    for (int i = 0 ; i < mem_pages ; i++) {
        int * thread_id_p = (int *) book_keeping_current_loc;
        *thread_id_p = -1;
        book_keeping_current_loc += size_int;
        int * offset_p = (int *) (book_keeping_current_loc);
        *offset_p = 0;
        book_keeping_current_loc += size_int;
    }
 //   display(mem);
}

//void * myallocate(int size , short req_type) {
void* myallocate(int size , int thread_id , short req_type) {
    if (size > page_size) {
        return NULL;
    }
    thread_id = GetThreadId();
    //int thread_id = 1;
    int page = GetPage(thread_id , size);
    if (page == -1) {
        return NULL;
    }
    void * ptr = Allocate(page , size , thread_id);
    return ptr;
}

int GetPage(int thread_id , int size) {
    char * mem = GetMemoryLoc();
    int free_page = -1;
    int increment = sizeof(book_keep);
    int size_int = sizeof(int);
    int free_space = 0;
    char * book_keeping_current_loc = book_keeping_start_loc;
    for (int i = 0 ; i < mem_pages ; i++) {
        free_space = GetFreeSpace(i);
        int thread = *(int*)(book_keeping_current_loc);
        if (thread == thread_id) {
            printf("Page = %d , size = %d , free space = %d \n" , i , size , free_space);
            if (free_space > size) {
                printf("returning page = %d \n" , i);
                return i;
            }
        }
        if (free_page == -1) {
            int page_offset = *(int*)(book_keeping_current_loc + size_int);
            if (page_offset == 0) {
                printf("Page = %d , size = %d , free space = %d \n" , i , size , free_space);
                free_page = i;
            }
        }
        book_keeping_current_loc += increment;
    }
    printf("returning page = %d\n" , free_page);
    return free_page;
}    
          
int GetFreeSpace(int page) {
    int size_int = sizeof(int);
    int book_keep_size = sizeof(book_keep);  
    char * book_keep_loc = book_keeping_start_loc + page*book_keep_size;
    int free_space_start = *(int *)(book_keep_loc + size_int);
    int free_space_end = page_size - size_int; 
    return free_space_end - free_space_start;
}

void * Allocate(int page , int size , int thread_id) {
    int size_int = sizeof(int);
    int book_keep_size = sizeof(book_keep);
    char * book_keep_loc = book_keeping_start_loc + page*book_keep_size;
    int * offset_loc = (int*)(book_keep_loc + size_int);
    int offset = *offset_loc;
    char * page_loc =  book_keeping_start_loc + (book_keeping_pages + page)*page_size;
    char * page_mem_loc = page_loc + offset;
    int * block_size_loc = (int*)(page_mem_loc + size);
    *block_size_loc = size/size_int;
    offset = offset + size + size_int;
    *offset_loc = offset;
    int * thread_id_loc = (int*)book_keep_loc;
    *thread_id_loc = thread_id;
    return (void*)page_mem_loc;
}

/*int main() {
    mem_manager_init();
    int * arr = (int*)myallocate(10*sizeof(int) , 1 , 2);
    int * arr1 = (int*)myallocate(500*sizeof(int) , 1 , 2);
    int * arr2 = (int*)myallocate(900*sizeof(int) , 2 , 2);
    int * arr4 = (int*)myallocate(900*sizeof(int) , 3 , 2);
    int * arr5 = (int*)myallocate(600*sizeof(int) , 3 , 2);
    return 0;
}*/       
