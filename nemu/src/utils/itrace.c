#include <common.h>
#define MAX_RING_BUF 2

typedef struct {
    uint32_t pc;
    uint32_t inst;
} ItraceNode;

ItraceNode iringbuf[MAX_RING_BUF];
int cur = 0;
bool full = false;

void disassemble(char *str, int size, uint64_t pc, uint8_t *code, int nbyte);

void itrace_add(uint32_t pc, uint32_t inst) {
    iringbuf[cur].pc = pc;
    iringbuf[cur].inst = inst;
    cur = (cur + 1) % MAX_RING_BUF;
    full = full || cur == 0;
}

void itrace_display() {
    if (!cur && !full) return;
    int i = full ? cur : 0;
    int end = cur;
    printf("===== the nearest %d instructions =====\n", MAX_RING_BUF);
    char longbuf[128];
    do {
        char *p = longbuf;
        p += snprintf(p, sizeof(longbuf), FMT_WORD ": %08x ", iringbuf[i].pc, iringbuf[i].inst);
        disassemble(p, longbuf + sizeof(longbuf) - p, iringbuf[i].pc, (uint8_t *)&iringbuf[i].inst, 4);
        puts(longbuf);
    } while ((i = (i + 1) % MAX_RING_BUF) != end);
    printf("=======================================\n");
}
