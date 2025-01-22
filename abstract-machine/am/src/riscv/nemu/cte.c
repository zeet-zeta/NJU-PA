#include <am.h>
#include <riscv/riscv.h>
#include <klib.h>

static Context* (*user_handler)(Event, Context*) = NULL;
extern void __am_switch(Context *);
extern void __am_get_cur_as(Context *);

#define USER_ECALL 11
// 这个参数来源于a0寄存器，参见trap.S
Context* __am_irq_handle(Context *c) {
  printf("__am_irq_handle\n");
  __am_get_cur_as(c);
  if (user_handler) {
    Event ev = {0};
    switch (c->mcause) {
      case USER_ECALL: 
        if (c->GPR1 == -1) {
          ev.event = EVENT_YIELD;
        }
        else {
          ev.event = EVENT_SYSCALL;
        }
        break;
      default: ev.event = EVENT_ERROR; break;
    }

    c = user_handler(ev, c);
    assert(c != NULL);
  }
  __am_switch(c);
  return c;
}

extern void __am_asm_trap(void);

bool cte_init(Context*(*handler)(Event, Context*)) {
  // initialize exception entry
  // 所以在nemu中直接返回cpu.mtvec
  asm volatile("csrw mtvec, %0" : : "r"(__am_asm_trap));

  // register event handler
  // 注册事件处理函数，在nanos里面调用的是cte_init(do_event);
  user_handler = handler;

  return true;
}

Context *kcontext(Area kstack, void (*entry)(void *), void *arg) {
  //Area [start, end)
  Context *c = (Context *)((uint8_t *)kstack.end - sizeof(Context));
  c->mepc = (uintptr_t)entry;
  c->mstatus = 0x1800;
  c->GPR2 = (uintptr_t)arg; //给f函数传参
  return c;
}

void yield() {
#ifdef __riscv_e
  asm volatile("li a5, -1; ecall");
#else
  asm volatile("li a7, -1; ecall"); //编号
#endif
}

bool ienabled() {
  return false;
}

void iset(bool enable) {
}
