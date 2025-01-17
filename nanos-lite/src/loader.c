#include <proc.h>
#include <elf.h>

#ifdef __LP64__
# define Elf_Ehdr Elf64_Ehdr
# define Elf_Phdr Elf64_Phdr
#else
# define Elf_Ehdr Elf32_Ehdr
# define Elf_Phdr Elf32_Phdr
#endif

extern size_t ramdisk_read(void *buf, size_t offset, size_t len);
extern void ramdisk_write(const void *buf, size_t offset, size_t len);

static uintptr_t loader(PCB *pcb, const char *filename) {
  Elf_Ehdr ehdr;
  Elf_Phdr phdr;
  size_t offset = 0;

  ramdisk_read(&ehdr, offset, sizeof(Elf_Ehdr));
  offset += sizeof(Elf_Ehdr);

  for (int i = 0; i < ehdr.e_phnum; i++) {
    ramdisk_read(&phdr, ehdr.e_phoff + i * ehdr.e_phentsize, sizeof(Elf_Phdr));
    if (phdr.p_type == PT_LOAD) {
      ramdisk_read((void *)phdr.p_vaddr, phdr.p_offset, phdr.p_filesz);
      if (phdr.p_filesz < phdr.p_memsz) {
        memset((void *)(phdr.p_vaddr + phdr.p_filesz), 0, phdr.p_memsz - phdr.p_filesz);
      }
    }
  }
  return ehdr.e_entry;
}

void naive_uload(PCB *pcb, const char *filename) {
  uintptr_t entry = loader(pcb, filename);
  Log("Jump to entry = %p", entry);
  ((void(*)())entry) (); //调用刚加载的程序
}

