#include <memory.h>

static void *pf = NULL;

void* new_page(size_t nr_page) {
  // void *old = pf;
  pf += nr_page * PGSIZE;
  // printf("new_page: %p\n", old);
  return pf;
}

#ifdef HAS_VME
static void* pg_alloc(int n) {
  void *new = new_page(n / PGSIZE);
  memset(new, 0, n);
  return new;
}
#endif

void free_page(void *p) {
  panic("not implement yet");
}

/* The brk() system call handler. */
int mm_brk(uintptr_t brk) {
  return 0;
}

void init_mm() {
  pf = (void *)ROUNDUP(heap.start, PGSIZE);
  Log("free physical pages starting from %p", pf);

#ifdef HAS_VME
  vme_init(pg_alloc, free_page);
#endif
}
