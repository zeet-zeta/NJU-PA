#include <proc.h>

#define MAX_NR_PROC 4

static PCB pcb[MAX_NR_PROC] __attribute__((used)) = {};
static PCB pcb_boot = {};
PCB *current = NULL;

extern void context_uload(PCB *pcb, const char *filename, char *const argv[], char *const envp[]);
extern void naive_uload(PCB *pcb, const char *filename);

void hello_fun(void *arg) {
  int j = 1;
  while (1) {
    //bug的原因在于我尝试*(int *)arg，就会将读取0x00000001的内存，而不是arg的值
    // printf("Hello! ");
    j ++;
    if (j % 100000 == 0) {
      printf("Hello! ");
    }
    yield();
  }
}
void context_kload(PCB *pcb, void (*entry)(void *), void *arg) {
  pcb->cp = kcontext((Area){pcb->stack, pcb + 1}, entry, arg);
}

Context *schedule(Context *prev) {
  printf("[0] : %p [1] : %p\n", (&pcb[0])->cp->pdir, (&pcb[1])->cp->pdir);
  current->cp = prev;
  current = (current == &pcb[0] ? &pcb[1] : &pcb[0]);
  // current = &pcb[0];
  return current->cp;
}

void switch_boot_pcb() {
  current = &pcb_boot;
}

void init_proc() {
  context_kload(&pcb[0], hello_fun, (void *)1);
  
  char *argv_example[] = {"/bin/pal", NULL};
  char *envp_example[] = {NULL};
  //数组的指针本身是常量，不能改变指向的地址，但指针指向的结果（字符串内容）可以是常量或非常量，具体取决于如何定义
  context_uload(&pcb[1], "/bin/pal", argv_example, envp_example);
  switch_boot_pcb();

  Log("Initializing processes...");

  // load program here

}

