#ifndef MY_MEMORY_MANAGER
#define MY_MEMORY_MANAGER

char * GetMemoryLoc();
int mem_manager_init();
void* myallocate(int , int , short);
void* Allocate(int , int , int);
int GetPage(int , int);
int GetFreeSpace(int);

#endif
