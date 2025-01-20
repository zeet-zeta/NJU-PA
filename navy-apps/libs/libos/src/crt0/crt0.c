#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

int main(int argc, char *argv[], char *envp[]);
extern char **environ;
void call_main(uintptr_t *args) {
  int argc = (int)args[0];
  char **argv = (char **)(args + 1);
  char **envp = (char **)(args + 1 + argc + 1);
  printf("argc = %d\n", argc);
  for (int i = 0; i < argc; i++) {
    printf("argv[%d] = %s\n", i, argv[i]);
  }
  environ = envp;
  exit(main(argc, argv, envp));
  assert(0);
}
