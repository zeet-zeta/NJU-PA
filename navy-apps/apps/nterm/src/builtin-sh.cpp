#include <nterm.h>
#include <stdarg.h>
#include <unistd.h>
#include <SDL.h>

char handle_key(SDL_Event *ev);

static void sh_printf(const char *format, ...) {
  static char buf[256] = {};
  va_list ap;
  va_start(ap, format);
  int len = vsnprintf(buf, 256, format, ap);
  va_end(ap);
  term->write(buf, len);
}

static void sh_banner() {
  sh_printf("Built-in Shell in NTerm (NJU Terminal)\n\n");
}

static void sh_prompt() {
  sh_printf("sh> ");
}

static void sh_handle_cmd(const char *cmd) {
  setenv("PATH", "/bin", 0);
  char line_copy[32];
  strcpy(line_copy, cmd);
  int len = strlen(line_copy);
  printf("cmd: %s\n", cmd);
  printf("%d\n", len);
  line_copy[len - 1] = '\0';
  int argc = 0;
  char *argv[16] = {};
  for (char *p = strtok(line_copy, " "); p; p = strtok(NULL, " ")) {
    argv[argc++] = p;
  }
  argv[argc] = NULL;
  execvp(argv[0], argv);
  sh_printf("sh: command not found: %s\n", cmd);
}

void builtin_sh_run() {
  sh_banner();
  sh_prompt();

  while (1) {
    SDL_Event ev;
    if (SDL_PollEvent(&ev)) {
      if (ev.type == SDL_KEYUP || ev.type == SDL_KEYDOWN) {
        const char *res = term->keypress(handle_key(&ev));
        if (res) {
          sh_handle_cmd(res);
          sh_prompt();
        }
      }
    }
    refresh_terminal();
  }
}
