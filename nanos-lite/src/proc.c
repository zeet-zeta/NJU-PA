#include <proc.h>

#define MAX_NR_PROC 4

static PCB pcb[MAX_NR_PROC] __attribute__((used)) = {};
static PCB pcb_boot = {};
PCB *current = NULL;

extern void naive_uload(PCB *pcb, const char *filename);

void hello_fun(void *arg) {
  int j = 1;
  while (1) {
    Log("Hello World from Nanos-lite with arg '%d' for the %dth time!", *(int *)arg, j);
    j ++;
    yield();
  }
}
void context_kload(PCB *pcb, void (*entry)(void *), void *arg) {
  //查看entry的地址
  pcb->cp = kcontext((Area){pcb->stack, pcb + 1}, entry, arg);
}

Context *schedule(Context *prev) {
  current->cp = prev;
  current = (current == &pcb[0] ? &pcb[1] : &pcb[0]);
  return current->cp;
}

void switch_boot_pcb() {
  current = &pcb_boot;
}

// static void f(void *arg) {
//   while (1) {
//     putch("?AB"[(uintptr_t)arg > 2 ? 0 : (uintptr_t)arg]);
//     for (int volatile i = 0; i < 100000; i++) ;
//     yield();
//   }
// }

void init_proc() {
  // context_kload(&pcb[0], hello_fun, (void *)1);
  // context_kload(&pcb[1], hello_fun, (void *)2);
  pcb[0].cp = kcontext((Area) { pcb[0].stack, &pcb[0] + 1 }, hello_fun, (void *)1L);
  pcb[1].cp = kcontext((Area) { pcb[1].stack, &pcb[1] + 1 }, hello_fun, (void *)2L);
  switch_boot_pcb();

  Log("Initializing processes...");
  yield();
  naive_uload(NULL, "/bin/menu");

  // load program here

}

