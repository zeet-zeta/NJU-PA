#include <common.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <elf.h>
#include <fcntl.h>
#include <unistd.h>

#define TYPE_CALL 0
#define TYPE_RET 1
#define FUNCTION_MAX_NUM 200

typedef struct {
    char name[128];
    Elf32_Addr address;
    Elf32_Word size;
} FunctionInfo;

typedef struct node {
    uint32_t pc;
    FunctionInfo func;
    int type;
    struct node *next;
} FtraceNode;

static int function_ctr = 0;
static FunctionInfo function_table[FUNCTION_MAX_NUM];
static FtraceNode *head = NULL;
static FtraceNode *tail = NULL;

void parse_symbol_table(int fd, Elf32_Ehdr *ehdr) {
    Elf32_Shdr *shdr;
    Elf32_Shdr *symtab = NULL;
    Elf32_Shdr *strtab = NULL;
    Elf32_Sym *symbols;
    char *strtab_data;
    size_t i;
    int read_ret;

    shdr = malloc(sizeof(Elf32_Shdr) * ehdr->e_shnum);
    lseek(fd, ehdr->e_shoff, SEEK_SET);
    read_ret = read(fd, shdr, sizeof(Elf32_Shdr) * ehdr->e_shnum);

    for (i = 0; i < ehdr->e_shnum; i++) {
        if (shdr[i].sh_type == SHT_SYMTAB) {
            symtab = &shdr[i];
        }
        if (shdr[i].sh_type == SHT_STRTAB) {
            strtab = &shdr[i];
            break; //会有两个strtab
        }
    }

    symbols = malloc(symtab->sh_size);
    lseek(fd, symtab->sh_offset, SEEK_SET);
    read_ret = read(fd, symbols, symtab->sh_size);

    strtab_data = malloc(strtab->sh_size);
    lseek(fd, strtab->sh_offset, SEEK_SET);
    read_ret = read(fd, strtab_data, strtab->sh_size);

    if (read_ret < 0) {
        Log("read err");
    }
    for (i = 0; i < symtab->sh_size / sizeof(Elf32_Sym); i++) {
        if (ELF32_ST_TYPE(symbols[i].st_info) == STT_FUNC) { 
            strcpy(function_table[function_ctr].name, &strtab_data[symbols[i].st_name]);
            function_table[function_ctr].size = symbols[i].st_size;
            function_table[function_ctr].address = symbols[i].st_value;
            function_ctr++;
        }
    }
    free(symbols);
    free(strtab_data);
    free(shdr);
}

int get_function_index(uint32_t addr) {
    for (int i = 0; i < function_ctr; i++) {
        if (addr >= function_table[i].address && addr < function_table[i].address + function_table[i].size) {
            return i;
        }
    }
    return -1;
}

void append(uint32_t pc, uint32_t dst, int type) {
    FtraceNode *new_node = malloc(sizeof(FtraceNode));
    new_node->pc = pc;
    new_node->type = type;
    new_node->func = function_table[get_function_index(dst)];
    if (head == NULL) {
        head = new_node;
        tail = new_node;
    } else {
        tail->next = new_node;
        tail = new_node;
    }
}

void ftrace_jal(int rd, uint32_t pc, uint32_t dst) {
    if (rd == 1) {
        append(pc, dst, TYPE_CALL);
    }
}

//riscv中没有真正的ret命令
void ftrace_jalr(int rd, uint32_t pc, uint32_t dst, uint32_t inst) {
    if (inst == 0x00008067) {
        append(pc, dst, TYPE_RET);
    } else if (rd == 1) {
        append(pc, dst, TYPE_CALL);
    }
}

void init_elf(char *elffile) {
    int fd = open(elffile, O_RDONLY);
    Elf32_Ehdr ehdr;
    if (read(fd, &ehdr, sizeof(ehdr)) < 0) {
        Log("read err");
    };
    if (memcmp(ehdr.e_ident, ELFMAG, SELFMAG) != 0) {
        fprintf(stderr, "Not a valid ELF file.\n");
        close(fd);
    }
    parse_symbol_table(fd, &ehdr);
    close(fd);
}

void ftrace_display() {
    int depth = 0;
    const char *type[] = {"call", "ret "};
    FtraceNode *cur = head;
    while(cur != NULL) {
        printf("0x%x: ", cur->pc);
        if (cur->type == TYPE_RET) depth--;
        for (int i = 0; i < depth * 2; i++) putchar(' ');
        if (cur->type == TYPE_CALL) depth++;
        printf("%s [%s@0x%x]\n",  type[cur->type], cur->func.name, cur->func.address);
        cur = cur->next;
    }
}