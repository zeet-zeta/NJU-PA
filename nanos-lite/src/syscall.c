#include <common.h>
#include "syscall.h"
#include "fs.h"
#include <sys/time.h>
#include "proc.h"

extern void context_uload(PCB *pcb, const char *filename, char *const argv[], char *const envp[]);
extern void naive_uload(PCB *pcb, const char *filename);
extern void switch_boot_pcb();
extern int mm_brk(uintptr_t brk);

int sys_gettimeofday(struct timeval *tv, struct timezone *tz) {
  uint64_t us = io_read(AM_TIMER_UPTIME).us;
  tv->tv_sec = us / 1000000;
  tv->tv_usec = us % 1000000;
  return 0;
}


int sys_execve(const char *pathname, const char *argv[], const char *envp[]) {
  // naive_uload(NULL, pathname);
  printf("sys_execve: %s\n", pathname);
  context_uload(current, pathname, (char * const *)argv, NULL);
  switch_boot_pcb();
  yield();
  return 0;
}

void do_syscall(Context *c) {
  uintptr_t a[4];
  a[0] = c->GPR1;
  a[1] = c->GPR2;
  a[2] = c->GPR3;
  a[3] = c->GPR4;

  switch (a[0]) {
    case SYS_exit:
      // naive_uload(NULL, "/bin/menu"); c->GPRx = 0; break;
      halt(0);break;
    case SYS_yield:
      printf("SYS_yield \n");yield(); c->GPRx = 0; break;
    case SYS_write:
      c->GPRx = fs_write(a[1], (void *)a[2], a[3]); break;
    case SYS_read:
      c->GPRx = fs_read(a[1], (void *)a[2], a[3]); break;
    case SYS_open:
      c->GPRx = fs_open((char *)a[1], a[2], a[3]); break;
    case SYS_close:
      c->GPRx = fs_close(a[1]); break;
    case SYS_brk:
      c->GPRx = mm_brk(a[1]); break;
    case SYS_lseek:
      c->GPRx = fs_lseek(a[1], a[2], a[3]); break;
    case SYS_gettimeofday:
      c->GPRx = sys_gettimeofday((struct timeval *)a[1], (struct timezone *)a[2]); break;
    case SYS_execve:
      c->GPRx = sys_execve((const char *)a[1], (const char **)a[2], (const char **)a[3]); break;
    default: panic("Unhandled syscall ID = %d", a[0]);
  }
}
