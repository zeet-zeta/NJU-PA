#include <common.h>
#include "syscall.h"

#define SYS_YIELD 1
void do_syscall(Context *c) {
  uintptr_t a[4];
  a[0] = c->GPR1;

  switch (a[0]) {
    case SYS_YIELD: yield(); c->GPRx = 0; break;
    default: panic("Unhandled syscall ID = %d", a[0]);
  }
}
