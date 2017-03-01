#ifndef MY_MEMORY_MANAGER
#define MY_MEMORY_MANAGER

typedef enum req_type
{
    LIBRARY,
    USER
}req_type;

char * GetMemoryLoc();
int mem_manager_init();
void* myallocate(int , req_type);
void* Allocate(int , int , int);
int GetPage(int , int);
int GetFreeSpace(int);
int FindEmptySlot();
int FindSlot(int , int);
int CopyToSwap(int , int);
int SwapOutAndFetch(int , int , int);
void ProtectContextPages();
void UnprotectContextPages();
void UnprotectPage(int);
void MemoryHandlerInit();
int GetPageFromAddress(char * , char*);
void FreeMemoryPages();
void * LibMemAllocate(int);
char * GetLatestLibAddress(int , int*);
char * GetLatestUserAddress(int , int*);
char * GetAddressAndLength(char*, int, int*);
void mydeallocate(void* , req_type);
void LibMemDeallocate(void*);
void UserMemDeallocate(void*);
void ResetLibOffset(int , int);
void ResetUserOffset(int , int);

#endif
