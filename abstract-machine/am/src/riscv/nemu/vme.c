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

#define VA_OFFSET(addr) (addr & 0x00000FFF) //提取虚拟地址的低 12 位，即在页面内的偏移。
#define VA_VPN_0(addr)  ((addr >> 12) & 0x000003FF) //提取虚拟地址的中间 10 位，即一级页号
#define VA_VPN_1(addr)  ((addr >> 22) & 0x000003FF) //提取虚拟地址的高 10 位，即二级页号
 
#define PA_OFFSET(addr) (addr & 0x00000FFF)//提取物理地址的低 12 位，即在页面内的偏移
#define PA_PPN(addr)    ((addr >> 12) & 0x000FFFFF)//提取物理地址的高 20 位，即物理页号
#define PTE_PPN 0xFFFFF000 // 31 ~ 12
void map(AddrSpace *as, void *va, void *pa, int prot) {
 
  uintptr_t va_trans = (uintptr_t) va;
  uintptr_t pa_trans = (uintptr_t) pa;
 
  assert(PA_OFFSET(pa_trans) == 0);
  assert(VA_OFFSET(va_trans) == 0);
 
  //提取虚拟地址的二级页号和一级页号，以及物理地址的物理页号
  uint32_t ppn = PA_PPN(pa_trans);
  uint32_t vpn_1 = VA_VPN_1(va_trans);
  uint32_t vpn_0 = VA_VPN_0(va_trans);
 
  //获取地址空间的页表基址和一级页表的目标位置
  PTE * page_dir_base = (PTE *) as->ptr;
  PTE * page_dir_target = page_dir_base + vpn_1;
  
  //如果一级页表中的页表项的地址(二级页表的基地址)为空，创建并填写页表项
  if (!(*page_dir_target & PTE_V)) { 
    //通过 pgalloc_usr 分配一页物理内存，作为二级页表的基地址
    PTE * page_table_base = (PTE *) pgalloc_usr(PGSIZE);
    //将这个基地址填写到一级页表的页表项中，同时设置 PTE_V 表示这个页表项是有效的。
    *page_dir_target = ((PTE) page_table_base) | PTE_V;
    //计算在二级页表中的页表项的地址
    PTE * page_table_target = page_table_base + vpn_0;
    //将物理页号 ppn 左移 12 位，即去掉低 12 位的偏移，与权限标志 PTE_V | PTE_R | PTE_W | PTE_X 组合，填写到二级页表的页表项中。
    *page_table_target = (ppn << 12) | PTE_V | PTE_R | PTE_W | PTE_X;
  } else {
    //取得一级页表项的内容，然后 & PTE_PPN 通过按位与操作提取出页表的基地址，提取高20位，低 12 位为零
    PTE * page_table_base = (PTE *) ((*page_dir_target) & PTE_PPN);
    //通过加上 vpn_0 计算得到在二级页表中的目标项的地址
    PTE * page_table_target = page_table_base + vpn_0;
    //将物理页号 ppn 左移 12 位，即去掉低 12 位的偏移，与权限标志 PTE_V | PTE_R | PTE_W | PTE_X 组合，填写到二级页表的目标项中。
    *page_table_target = (ppn << 12) | PTE_V | PTE_R | PTE_W | PTE_X;
  }
 
}

Context *ucontext(AddrSpace *as, Area kstack, void *entry) {
  Context *c = (Context *)((uint8_t *)kstack.end - sizeof(Context));
  c->mepc = (uintptr_t)entry;
  c->pdir = (void *)as;
  return c;
}
