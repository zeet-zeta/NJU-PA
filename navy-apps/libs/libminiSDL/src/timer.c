#include <NDL.h>
#include <sdl-timer.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  SDL_NewTimerCallback callback;
  void *param;
  uint32_t interval;
  uint32_t last;
} Timer;

static Timer *timer[32] = {0};

SDL_TimerID SDL_AddTimer(uint32_t interval, SDL_NewTimerCallback callback, void *param) {
  for (int i = 0; i < 32; i++) {
    if (timer[i] == NULL) {
      timer[i] = (Timer *)malloc(sizeof(Timer));
      timer[i]->callback = callback;
      timer[i]->param = param;
      timer[i]->interval = interval;
      timer[i]->last = NDL_GetTicks();
      return (SDL_TimerID)timer[i];
    }
  }
  return NULL;
}

int SDL_RemoveTimer(SDL_TimerID id) {
  for (int i = 0; i < 32; i++) {
    if (timer[i] == (Timer *)id) {
      free(timer[i]);
      timer[i] = NULL;
      return 1;
    }
  }
  return 0;
}

uint32_t SDL_GetTicks() {
  return NDL_GetTicks();
}

void SDL_Delay(uint32_t ms) {
  uint32_t start = NDL_GetTicks();
  while (NDL_GetTicks() - start < ms);
}
