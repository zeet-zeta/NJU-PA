#include <am.h>
#include <nemu.h>
#include <klib.h>

static AddrSpace kas = {};
static void* (*pgalloc_usr)(int) = NULL;
static void (*pgfree_usr)(void*) = NULL;
static int vme_enable = 0;

static Area segments[] = {      // Kernel memory mappings
  NEMU_PADDR_SPACE
};

#define USER_SPACE RANGE(0x40000000, 0x80000000)

//指向页表基地址
static inline void set_satp(void *pdir) {
  
  uintptr_t mode = 1ul << (__riscv_xlen - 1);
  printf("set base addr: %p \n", pdir);
  asm volatile("csrw satp, %0" : : "r"(mode | ((uintptr_t)pdir >> 12)));
}

static inline uintptr_t get_satp() {
  uintptr_t satp;
  asm volatile("csrr %0, satp" : "=r"(satp));
  return satp << 12;
}

bool vme_init(void* (*pgalloc_f)(int), void (*pgfree_f)(void*)) {
  pgalloc_usr = pgalloc_f;
  pgfree_usr = pgfree_f;

  kas.ptr = pgalloc_f(PGSIZE); //页表基地址

  int i;
  for (i = 0; i < LENGTH(segments); i ++) {
    void *va = segments[i].start;
    for (; va < segments[i].end; va += PGSIZE) {
      map(&kas, va, va, 1); //映射内核空间，恒等映射
    }
  }

  set_satp(kas.ptr);
  vme_enable = 1;

  return true;
}

//创一个用户程序的页，将内核空间映射到用户空间
void protect(AddrSpace *as) {
  PTE *updir = (PTE*)(pgalloc_usr(PGSIZE));
  as->ptr = updir;
  as->area = USER_SPACE;
  as->pgsize = PGSIZE;
  // map kernel space
  memcpy(updir, kas.ptr, PGSIZE);
}

void unprotect(AddrSpace *as) {
}

void __am_get_cur_as(Context *c) {
  c->pdir = (vme_enable ? (void *)get_satp() : NULL);
}

void __am_switch(Context *c) {
  if (vme_enable && c->pdir != NULL) {
    set_satp(c->pdir);
  }
}

/*
  Virtual Address           |  VPN[1]  |  VPN[0]  |   offset   |  -----> 32 bits
                            +----------+----------+------------+
                             (10 bits)  (10 bits)    (12 bits)

  Physical Address        +------------+----------+------------+
                          |   PPN[1]   |  PPN[0]  |   offset   |  -----> 34 bits
                          +------------+----------+------------+
                             (12 bits)   (10 bits)    (12 bits)

                    +------------+----------+--+-+-+-+-+-+-+-+-+
  Page Table Entry  |   PPN[1]   |  PPN[0]  |  |D|A|G|U|X|W|R|V|  -----> 32 bits
                    +------------+----------+--+-+-+-+-+-+-+-+-+
                       (12 bits)   (10 bits) |
                                             `- RSW (2 bits)
*/

// static inline PTE* page_walk(AddrSpace *as, void *va, int prot) {
//   PTE *first_pte = (PTE *)as->ptr + ((uintptr_t)va >> 22); //一个PTE是4字节
//   if ((*first_pte & PTE_V) == 0) { //缺页
//     void *new = pgalloc_usr(PGSIZE);
//     *first_pte = ((uintptr_t)new >> 2) | prot;
//   }
//     printf("pagewalk: %x ", *first_pte);
//   PTE *second_pte = (PTE *)(((*first_pte) >> 10 << 12)) + (((uintptr_t)va >> 12) & 0x3ff);
//   return second_pte;
// }


// //用于将地址空间as中虚拟地址va所在的虚拟页, 以prot的权限映射到pa所在的物理页
// void map(AddrSpace *as, void *va, void *pa, int prot) {
//   va = (void *)((uintptr_t)va & ~0xfff);
//   pa = (void *)((uintptr_t)pa & ~0xfff);
//   PTE *pgdir = page_walk(as, va, prot);
//   *pgdir = (((uintptr_t)pa) >> 2) | prot;
//   printf("va: %p -> pa: %p ", va, pa);
// }

#define PTE_PPN_MASK (0xFFFFFC00u)
#define PTE_PPN(x) (((x) & PTE_PPN_MASK) >> 10)
#define PGT1_ID(val) (val >> 22)
#define PGT2_ID(val) ((val & 0x3fffff) >> 12)

void map(AddrSpace *as, void *va, void *pa, int prot) {
    va = (void *)((int)va & ~0xfff);
    pa = (void *)((int)pa & ~0xfff);
    PTE *pte_1 = as->ptr + PGT1_ID((uintptr_t)va) * 4;          // 与 4 做乘法，va 需要从 void * 转成 uint 或 int
    if (!(*pte_1 & PTE_V)) {
        void *allocated_page = pgalloc_usr(PGSIZE);
        // 构造 PTE
        *pte_1 = ((uintptr_t)allocated_page >> 2) | prot;  //  | PTE_V 也可以, 但我觉得不规范
    }
    PTE *pte_2 = (PTE *)((PTE_PPN(*pte_1) << 12) + PGT2_ID((uintptr_t)va) * 4);
    // 构造PTE，pa 的低 12 位在开始就已清零，现在创建 22 位的 PPN，往右移动 2 位。然后构造低 10 位的控制位
    //*pte_2 = ((uintptr_t)pa >> 2) | PTE_V | PTE_R | PTE_W | PTE_X | (prot ? PTE_U : 0);
    *pte_2 = ((uintptr_t)pa >> 2) | prot;
}

Context *ucontext(AddrSpace *as, Area kstack, void *entry) {
  Context *c = (Context *)((uint8_t *)kstack.end - sizeof(Context));
  c->mepc = (uintptr_t)entry;
  c->pdir = (void *)as;
  return c;
}
