#include <proc.h>
#include <elf.h>
#include "fs.h"
#include "am.h"

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
  // ramdisk_read(&ehdr, 0, sizeof(Elf_Ehdr));
  assert(*(uint32_t *)ehdr.e_ident == 0x464c457f);
  assert(ehdr.e_machine == EXPECT_TYPE);

  for (int i = 0; i < ehdr.e_phnum; i++) {
    // ramdisk_read(&phdr, ehdr.e_phoff + i * ehdr.e_phentsize, sizeof(Elf_Phdr));
    fs_lseek(fd, ehdr.e_phoff + i * ehdr.e_phentsize, SEEK_SET);
    fs_read(fd, &phdr, sizeof(Elf_Phdr));
    if (phdr.p_type == PT_LOAD) {
      // ramdisk_read((void *)phdr.p_vaddr, phdr.p_offset, phdr.p_filesz);
      fs_lseek(fd, phdr.p_offset, SEEK_SET);
      fs_read(fd, (void *)phdr.p_vaddr, phdr.p_filesz);
      if (phdr.p_filesz < phdr.p_memsz) {
        memset((void *)(phdr.p_vaddr + phdr.p_filesz), 0, phdr.p_memsz - phdr.p_filesz);
      }
    }
  }
  fs_close(fd);
  return ehdr.e_entry;
}

void naive_uload(PCB *pcb, const char *filename) {
  uintptr_t entry = loader(pcb, filename);
  ((void(*)())entry) (); //调用刚加载的程序
}

void context_uload(PCB *pcb, const char *filename, char *const argv[], char *const envp[]) {
  uintptr_t entry = loader(pcb, filename);
  pcb->cp = ucontext(&(pcb->as), (Area){pcb->stack, pcb + 1}, (void *)entry);
  int argc = 0;
  while (argv[argc]) argc++;
  int envc = 0;
  while (envp[envc]) envc++;

  uintptr_t ustack_end = (uintptr_t)heap.end;
  uintptr_t ustack_top = ustack_end;
  char **argv_copy = malloc(argc * sizeof(char *));
  char **envp_copy = malloc(envc * sizeof(char *));

  for (int i = envc - 1; i >= 0; i--) {
    ustack_top -= strlen(envp[i]) + 1; // '\0'
    strcpy((char *)ustack_top, envp[i]);
    envp_copy[i] = (char *)ustack_top;
    // printf("envp_copy[%d] = %s\n", i, envp_copy[i]);
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
  pcb->cp->GPRx = ustack_top;
  free(argv_copy);
  free(envp_copy);
}
