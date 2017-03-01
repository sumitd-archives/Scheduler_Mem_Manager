#include<stdio.h>
#include<unistd.h>
#include "my_memory_manager.h"
#include "my_pthread_t.h"
#include <sys/mman.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
typedef struct book_keep
{
    int thread;
    int offset;
}book_keep;

typedef struct swap_info
{
    int thread;
    int page;
    int offset;
}swap_info;

typedef struct lib_book_keep
{
    int offset;
}lib_book_keep;

#define mem_size (8*1024*1024)
#define lib_pages 209
#define mem_pages 1465
#define swap_file_size (16*1024*1024)
#define swap_file_pages 4096
#define book_keeping_pages 4
#define library_mem (1024*1024)
#define default_page_size 4096

char mem[mem_size];
int page_size =  default_page_size;
char *book_keeping_start_loc;
char *user_mem_pages_start_loc;
char *lib_book_keep_start_loc;
char *lib_mem_pages_start_loc;
swap_info swap_info_table[swap_file_pages];

sigset_t sigs;
void MemoryHandler(int signum , siginfo_t* si , void* uc) {
    sigprocmask(SIG_BLOCK , &sigs , NULL);
    char * addr = (char*)si->si_addr;
    int page = GetPageFromAddress(addr , user_mem_pages_start_loc);
    UnprotectPage(page);
    int thread = GetThreadId();
    SwapOutAndFetch(page , page , thread);
    sigprocmask(SIG_UNBLOCK , &sigs , NULL);
}

char * GetMemoryLoc() {
    return mem;
}
 
int mem_manager_init() {
    MemoryHandlerInit();
    sigemptyset(&sigs);
    sigaddset(&sigs , SIGRTMIN);
    page_size = (int)sysconf(_SC_PAGE_SIZE);
    char * mem = GetMemoryLoc();
    int size_int = sizeof(int);
    int i,j;
    //library memory book keeping
    lib_book_keep_start_loc = mem;
    char* lib_book_keep_current = lib_book_keep_start_loc;
    for (i = 0; i < lib_pages; i++) {
        int *lib_offset_p = (int*)lib_book_keep_current;
        *lib_offset_p = 0;
        lib_book_keep_current += sizeof(lib_book_keep);
    } 
    lib_mem_pages_start_loc = lib_book_keep_current;

    //user mem book keeping and memory pages
    book_keeping_start_loc = mem + library_mem;
    int half_size = sizeof(book_keep)/2;
    char *book_keeping_current_loc = book_keeping_start_loc; 
    for (i = 0 ; i < mem_pages ; i++) {
        int * thread_id_p = (int *) book_keeping_current_loc;
        *thread_id_p = -1;
        book_keeping_current_loc += size_int;
        int * offset_p = (int *) (book_keeping_current_loc);
        *offset_p = 0;
        book_keeping_current_loc += size_int;
   }
    //set the first user mem page loc alligned with system page boundary
    int padding = page_size - (int)book_keeping_current_loc % page_size;
    user_mem_pages_start_loc = book_keeping_current_loc + padding;

    //initialize swap_info_table
    for (j = 0 ; j < swap_file_pages ; j++) {
        swap_info_table[j].thread = -1;
        swap_info_table[j].page = -1;
        swap_info_table[j].offset = 0;
    }
}

int SwapOutAndFetch(int s_page, int t_page, int t_thread) {
    char temp[page_size];
    int slot = FindSlot(t_page , t_thread);
    if (slot == -1) {
        return -1;
    }
    int offset = swap_info_table[slot].offset; 
    FILE * fp = fopen("swap.txt" , "r");
    fseek(fp , page_size*slot , SEEK_SET);
    int count = 0;
    int c;
    while(count < page_size) { 
        c = fgetc(fp);
        *(temp + count) = (char)c;
        ++count;
    }
    fclose(fp);       
    CopyToSwap(s_page, slot);
    char * page_loc =  user_mem_pages_start_loc + s_page*page_size;
    memcpy(page_loc, temp, page_size);
    //update the book keeping entry to reflect this
    char * book_keeping_entry = book_keeping_start_loc + s_page*sizeof(book_keep); 
    int * thread_id_p = (int*)book_keeping_entry;
    *thread_id_p = t_thread;
    int * offset_p = (int*)(book_keeping_entry + 4);
    *offset_p = offset;  
    return 0;
}

int CopyToSwap(int page, int slot) {
    if (slot == -1) { 
        //no slot provided . find empty slot
        slot = FindEmptySlot();    
        if (slot == -1) {
            //swap file is full. no swap available
            return -1;
        }
    }
    char * page_loc =  user_mem_pages_start_loc + page*page_size;
    FILE * fp = fopen("swap.txt" , "r+");
    fseek(fp , page_size*slot , SEEK_SET);
    fwrite(page_loc , 1 , page_size , fp);
    fclose(fp);
    char * book_keep_loc = book_keeping_start_loc + page*sizeof(book_keep);
    int thread = *(int*)book_keep_loc;
    int offset = *(int*)(book_keep_loc + 4);
    swap_info_table[slot].thread = thread;
    swap_info_table[slot].page = page;
    swap_info_table[slot].offset = offset;
    return 0;
}

int FindSlot(int page , int thread) {
    int ret = -1;
    int i ;
    for (i = 0 ; i < swap_file_pages ; i++) {
        if((swap_info_table[i].thread == thread) && 
          (swap_info_table[i].page == page)) {
            return i;
        }
    }
    return ret; 
}
        
int FindEmptySlot() {
    int ret = -1;
    int i;
    for (i = 0 ; i < swap_file_pages ; i++) {
        if(swap_info_table[i].thread == -1) {
            return i;
        }
    }
    return ret; 
}

void* myallocate(int size , req_type req) {
    sigprocmask(SIG_BLOCK , &sigs , NULL);
    if (size > page_size) {
        return NULL;
    }
    if (req == LIBRARY) {
        void* ptr = LibMemAllocate(size);
        sigprocmask(SIG_UNBLOCK , &sigs , NULL);
        return ptr;
    }
    
    int thread_id = GetThreadId();
    int page;
    page = GetPage(thread_id , size);
    if (page == -1) {
        page = 0;
        UnprotectPage(page);
        if (CopyToSwap(page , -1) == -1) {
            //swap file full . return null  
            return NULL;
        }
        //reset the book keeping informattion for this page
        char * book_keep_loc = book_keeping_start_loc + page*sizeof(book_keep);
        int* thread_p = (int*)book_keep_loc;
        *thread_p = -1;
        int* offset_p = (int*)(book_keep_loc + 4);
        *offset_p =0;
    } 
    void * ptr = Allocate(page , size , thread_id);
    sigprocmask(SIG_UNBLOCK , &sigs , NULL);
    return ptr;
    page = rand() % mem_pages;
    CopyToSwap(page , -1);
}

int GetPageFromAddress(char * addr , char * start_loc) {
    int byte_offset = (int)(addr - start_loc);
    int page = byte_offset/page_size;
    return page;
}

int GetPage(int thread_id , int size) {
    char * mem = GetMemoryLoc();
    int free_page = -1;
    int increment = sizeof(book_keep);
    int size_int = sizeof(int);
    int free_space = 0;
    char * book_keeping_current_loc = book_keeping_start_loc;
    int i;
    for (i = 0 ; i < mem_pages ; i++) {
        free_space = GetFreeSpace(i);
        int thread = *(int*)(book_keeping_current_loc);
        if (thread == thread_id) {
            if (free_space > size) {
                return i;
            }
        }
        if (free_page == -1) {
            int page_offset = *(int*)(book_keeping_current_loc + size_int);
            if (page_offset == 0) {
                free_page = i;
            }
        }
        book_keeping_current_loc += increment;
    }
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

void* LibMemAllocate(int size) {
    //return malloc(size);
    int i;
    char* lib_book_keep_current_loc = lib_book_keep_start_loc;    
    int* offset_p;
    int offset;
    for (i = 0; i < lib_pages; i++) {
        offset_p = (int*)lib_book_keep_current_loc;
        offset = *offset_p;
        if ((page_size - offset) >= size) {
            break;
        }
        lib_book_keep_current_loc += sizeof(lib_book_keep);
    }
    if (i == lib_pages) {
        return NULL;
    }
    char* page_loc = lib_mem_pages_start_loc + i*page_size;
    char* page_mem_loc = page_loc + offset;
    int* block_size_loc = (int*)(page_mem_loc + size);
    *block_size_loc = size;
    offset = offset + size + sizeof(int);
    *offset_p = offset;      
    return (void*)page_mem_loc;
}

void * Allocate(int page , int size , int thread_id) {
    int size_int = sizeof(int);
    int book_keep_size = sizeof(book_keep);
    char * book_keep_loc = book_keeping_start_loc + page*book_keep_size;
    int * offset_loc = (int*)(book_keep_loc + size_int);
    int offset = *offset_loc;
    char * page_loc =  user_mem_pages_start_loc + page*page_size;
    char * page_mem_loc = page_loc + offset;
    int * block_size_loc = (int*)(page_mem_loc + size);
    *block_size_loc = size;
    offset = offset + size + size_int;
    *offset_loc = offset;
    int * thread_id_loc = (int*)book_keep_loc;
    *thread_id_loc = thread_id;
    return (void*)page_mem_loc;
}

void ProtectContextPages() {
    int thread = GetThreadId();
    int i;
    for (i = 0; i < mem_pages; i++) {
        int * thread_p = (int*)(book_keeping_start_loc + i*sizeof(book_keep));
        if (*thread_p == thread) {
            //memprotect this page
            char * page_loc = user_mem_pages_start_loc + i*page_size;
            mprotect(page_loc , page_size , PROT_NONE);
        } 
    }
}

void FreeMemoryPages() {
    //Free memory pages
    int thread = GetThreadId();
    int i;
    for (i = 0; i < mem_pages; i++) {
        int * thread_p = (int*)(book_keeping_start_loc + i*sizeof(book_keep));
        if (*thread_p == thread) {
            *thread_p = -1;
            int* offset_p = (int*)(thread_p + 1);
            *offset_p = 0;
        }
    }
    //Free swap file pages
    for (i = 0; i < mem_pages; i++) {
        if(swap_info_table[i].thread == thread) {
            swap_info_table[i].thread = -1;
            swap_info_table[i].page = -1;
            swap_info_table[i].offset = 0;
        }
    }
}

void mydeallocate(void * addr , req_type type) {
    sigprocmask(SIG_BLOCK , &sigs , NULL);
    if (type == LIBRARY) {
        LibMemDeallocate(addr);
    }
    else {
        UserMemDeallocate(addr);
    }
    sigprocmask(SIG_UNBLOCK , &sigs , NULL);
}    

void LibMemDeallocate(void * addr) {
    //free(addr);
    int length;
    int page = GetPageFromAddress((char*)addr, lib_mem_pages_start_loc);
    char * latest_addr = GetLatestLibAddress(page , &length);
    if (latest_addr == (char*)addr) {
        ResetLibOffset(page , length + 4);
    }
}

void UserMemDeallocate(void * addr) {
    int length;
    int page = GetPageFromAddress((char*)addr, user_mem_pages_start_loc);
    char * latest_addr = GetLatestUserAddress(page , &length);
    if (latest_addr == (char*)addr) {
        ResetUserOffset(page , length + 4);
    }
}

void ResetLibOffset(int page , int length) {
    char * book_keep_loc = lib_book_keep_start_loc + page*sizeof(lib_book_keep);
    int * offset_p = (int*)book_keep_loc;
    *offset_p = *offset_p - length; 
}

void ResetUserOffset(int page , int length) {
    char * book_keep_loc = book_keeping_start_loc + page*sizeof(book_keep);
    int * offset_p = (int*)(book_keep_loc + 4);
    *offset_p = *offset_p - length; 
    if (*offset_p == 0) {
        int* thread_id = (int*)book_keep_loc;
        *thread_id = -1;
    }
}
    
char * GetLatestLibAddress(int page , int* length) {
    char * book_keep_loc = lib_book_keep_start_loc + page*sizeof(lib_book_keep);
    int offset = *(int *)book_keep_loc;
    char* lib_page_loc = lib_mem_pages_start_loc + page*page_size;
    return GetAddressAndLength(lib_page_loc, offset, length);
}

char * GetLatestUserAddress(int page , int* length) {
    char * book_keep_loc = book_keeping_start_loc + page*sizeof(book_keep);
    int offset = *(int *)(book_keep_loc + 4);
    char* user_page_loc = user_mem_pages_start_loc + page*page_size;
    return GetAddressAndLength(user_page_loc, offset, length);
}

char * GetAddressAndLength(char * page_loc, int offset , int* length) {
    char* page_offset_loc = page_loc + offset;
    int offset_len = *(int*)(page_offset_loc - 4);
    *length = offset_len;
    char * latest_addr = (char*)(page_offset_loc - 4 - offset_len);
    return latest_addr; 
}

void UnprotectContextPages() { 
    int thread = GetThreadId();
    printf("Unprotecting pages for thread_id = %d\n" , thread);
    int i;
    for (i = 0; i < mem_pages; i++) {
        int * thread_p = (int*)(book_keeping_start_loc + i*sizeof(book_keep));
        if (*thread_p == thread) {
            //memprotect this page
            char * page_loc = user_mem_pages_start_loc + i*page_size;
            mprotect(page_loc , page_size , PROT_READ|PROT_WRITE);
        } 
    }
}

void UnprotectPage(int page) {
    char * page_loc = user_mem_pages_start_loc + page*page_size;
    mprotect(page_loc , page_size , PROT_READ|PROT_WRITE);
}

void MemoryHandlerInit() {
    struct sigaction sa;
    int signum = SIGSEGV;
    sa.sa_flags=SA_SIGINFO;
    sa.sa_sigaction = MemoryHandler;
    sigemptyset(&sa.sa_mask);
    if (sigaction(signum , &sa , NULL) == -1) {
        perror("error in registering handler\n");
    }  
}
