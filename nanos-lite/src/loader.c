#include <proc.h>
#include <elf.h>
#include "fs.h"
#include "am.h"

#define PTE_V 0x01
#define PTE_R 0x02
#define PTE_W 0x04
#define PTE_X 0x08
#define PTE_U 0x10
#define PTE_A 0x40
#define PTE_D 0x80

#ifdef __LP64__
# define Elf_Ehdr Elf64_Ehdr
# define Elf_Phdr Elf64_Phdr
#else
# define Elf_Ehdr Elf32_Ehdr
# define Elf_Phdr Elf32_Phdr
#endif

#if defined(__ISA_AM_NATIVE__)
# define EXPECT_TYPE EM_X86_64
#elif defined(__ISA_X86__)
# define EXPECT_TYPE EM_X86_64
#elif defined(__ISA_MIPS32__)
# define EXPECT_TYPE EM_MIPS
#elif defined(__riscv)
# define EXPECT_TYPE EM_RISCV
#elif defined(__ISA_LOONGARCH32R__)
# define EXPECT_TYPE EM_LOONGARCH
#else
# error Unsupported ISA
#endif

extern size_t ramdisk_read(void *buf, size_t offset, size_t len);
extern size_t ramdisk_write(const void *buf, size_t offset, size_t len);

static uintptr_t loader(PCB *pcb, const char *filename) {
  Elf_Ehdr ehdr;
  Elf_Phdr phdr;
  int fd = fs_open(filename, 0, 0);

  fs_read(fd, &ehdr, sizeof(Elf_Ehdr));
  assert(*(uint32_t *)ehdr.e_ident == 0x464c457f);
  assert(ehdr.e_machine == EXPECT_TYPE);

  for (int i = 0; i < ehdr.e_phnum; i++) {
    fs_lseek(fd, ehdr.e_phoff + i * ehdr.e_phentsize, SEEK_SET);
    fs_read(fd, &phdr, sizeof(Elf_Phdr));
    if (phdr.p_type == PT_LOAD) {
      //虚拟地址版
      uintptr_t va = phdr.p_vaddr;
      int filesz = phdr.p_filesz;
      int memsz = phdr.p_memsz;
      int read_len = 0;
      void *pa = NULL;
      fs_lseek(fd, phdr.p_offset, SEEK_SET);


      if ((va & 0xfff) != 0) {
        //没对齐
        int offset = va & 0xfff;
        va &= ~0xfff;
        pa = new_page(1);
        read_len = (offset + filesz > PGSIZE) ? PGSIZE - offset : filesz;
        map(&(pcb->as), (void *)va, pa, PTE_R | PTE_W | PTE_X | PTE_V);
        fs_read(fd, pa + offset, read_len);

        va += PGSIZE;
        filesz -= read_len;
        memsz -= read_len;
      }

      while (filesz > 0) {
        pa = new_page(1);
        read_len = (filesz > PGSIZE) ? PGSIZE : filesz;
        map(&(pcb->as), (void *)va, pa, PTE_R | PTE_W | PTE_X | PTE_V);
        fs_read(fd, pa, read_len);
        va += PGSIZE;
        filesz -= read_len;
        memsz -= read_len;
      }

      if (read_len + memsz > PGSIZE) {
        memsz -= PGSIZE - read_len;
        while (memsz > 0) {
          pa = new_page(1);
          map(&(pcb->as), (void *)va, pa, PTE_R | PTE_W | PTE_X | PTE_V);
          va += PGSIZE;
          memsz -= PGSIZE;
        }
      }
      pcb->max_brk = va;
    }
  }
  fs_close(fd);
  // assert(0);
  return ehdr.e_entry;
}

void naive_uload(PCB *pcb, const char *filename) {
  uintptr_t entry = loader(pcb, filename);
  ((void(*)())entry) (); //调用刚加载的程序
}

void context_uload(PCB *pcb, const char *filename, char *const argv[], char *const envp[]) {
  // AddrSpace as = pcb->as;
  // 大错特错，这里的as是pcb->as的一个副本
  AddrSpace *as = &pcb->as;
  protect(as);
  uintptr_t entry = loader(pcb, filename);
  printf("entry: %x\n", entry);
  pcb->cp = ucontext(&(pcb->as), (Area){pcb->stack, pcb + 1}, (void *)entry);

  uintptr_t va_end = (uintptr_t)as->area.end;
  uintptr_t va_start = va_end - 32 * 1024;
  void *pa_start = new_page(1);
  for (uintptr_t va = va_start; va < va_end; va += PGSIZE) {
    void *pa = new_page(1);
    map(as, (void *)va, pa, PTE_R | PTE_W | PTE_X | PTE_V);
  }

  int argc = 0;
  if (argv == NULL) {
    argc = 0;
  } else {
    while (argv[argc] != NULL) argc++;
  }
  int envc = 0;
  if (envp == NULL) {
    envc = 0;
  } else {
    while (envp[envc] != NULL) envc++;
  }

  uintptr_t ustack_end = (uintptr_t)pa_start + PGSIZE * 10;
  uintptr_t ustack_top = ustack_end;
  //此处不能使用malloc,其中一个原因是malloc和new_page分配的空间是冲突的
  char *argv_copy[argc];
  char *envp_copy[envc];
  for (int i = envc - 1; i >= 0; i--) {
    ustack_top -= strlen(envp[i]) + 1; // '\0'
    strcpy((char *)ustack_top, envp[i]);
    envp_copy[i] = (char *)ustack_top;
  }
  for (int i = argc - 1; i >= 0; i--) {
    ustack_top -= strlen(argv[i]) + 1; // '\0'
    strcpy((char *)ustack_top, argv[i]);
    argv_copy[i] = (char *)ustack_top;
  }

  ustack_top -= (envc + 1) * sizeof(char *);
  memcpy((void *)ustack_top, envp_copy, envc * sizeof(char *));
  ustack_top -= (argc + 1) * sizeof(char *);
  memcpy((void *)ustack_top, argv_copy, argc * sizeof(char *));
  
  ustack_top -= sizeof(int);
  *(int *)ustack_top = argc;
  pcb->cp->GPRx = ustack_top - (ustack_end - va_end); //改成虚拟地址
  // assert(0);
}