#include <common.h>
#define MAX_RING_BUF 3

typedef struct {
    uint32_t pc;
    uint32_t inst;
} ItraceNode;

static ItraceNode iringbuf[MAX_RING_BUF];
static int cur = 0;
static bool is_full = false;

void disassemble(char *str, int size, uint64_t pc, uint8_t *code, int nbyte);

void itrace_add(uint32_t pc, uint32_t inst) {
    iringbuf[cur].pc = pc;
    iringbuf[cur].inst = inst;
    cur = (cur + 1) % MAX_RING_BUF;
    if (cur == 0 && !is_full) {
        is_full = true;
    }
}

void itrace_display() {
    if (cur == 0 && !is_full) return;
    char longbuf[128];
    for (int i = is_full ? cur : 0; (i + 1) % MAX_RING_BUF != cur; i = (i + 1) % MAX_RING_BUF) {
        printf("i=%d", i);
        char *p = longbuf;
        p += snprintf(p, sizeof(longbuf), FMT_WORD ": %08x ", iringbuf[i].pc, iringbuf[i].inst);
        disassemble(p, longbuf + sizeof(longbuf) - p, iringbuf[i].pc, (uint8_t *)&iringbuf[i].inst, 4);
        puts(longbuf);
        i = (i + 1) % MAX_RING_BUF;
    }
}
