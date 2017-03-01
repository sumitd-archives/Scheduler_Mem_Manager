#define _XOPEN_SOURCE_EXTENDED 1
#include <stdio.h>
#include <ucontext.h>
#include <stdbool.h>

void func(void);

int  x = 0;
ucontext_t context, *cp = &context;
bool called = false;

int main(void) {

  if (!x) {
    printf("getcontext has been called\n");
    getcontext(cp);
    if (!called) {
        called = true;
        func();
    }
  }
  else {
    printf("setcontext has been called\n");
  }
}

void func(void) {

  x++;
  setcontext(cp);

}
