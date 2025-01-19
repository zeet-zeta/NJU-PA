#include <NDL.h>
#include <SDL.h>
#include <string.h>

#define keyname(k) #k,

static const char *keyname[] = {
  "NONE",
  _KEYS(keyname)
};

int keynum = sizeof(keyname) / sizeof(keyname[0]);
static uint8_t keystate[sizeof(keyname) / sizeof(keyname[0])];
// static SDL_Event event_queue[32];
// static int head = 0, tail = 0;

int SDL_PushEvent(SDL_Event *ev) {
  // int next_tail = (tail + 1) % 32;
  // if (next_tail == head) return 0;
  // event_queue[tail] = *ev;
  // tail = next_tail;
  // return 1;
  return 0;
}

int SDL_PollEvent(SDL_Event *ev) {
  char buf[64];
  if (NDL_PollEvent(buf, sizeof(buf)) == 0) return 0;
  if (strncmp(buf, "kd", 2) == 0) {
    ev->type = SDL_KEYDOWN;
  } else {
    ev->type = SDL_KEYUP;
  }
  int len = strlen(buf);
  buf[len - 1] = '\0';
  for (int i = 0; i < keynum; i++) {
    if (strcmp(buf + 3, keyname[i]) == 0) {
      ev->key.keysym.sym = i;
      if (ev->type == SDL_KEYDOWN) {
        keystate[i] = 1;
      } else {
        keystate[i] = 0;
      }
      break;
    }
  }
  return 1;
}

int SDL_WaitEvent(SDL_Event *ev) {
  char buf[64];
  while (1) {
    if (NDL_PollEvent(buf, sizeof(buf)) == 0) continue;
    if (strncmp(buf, "kd", 2) == 0) {
      ev->type = SDL_KEYDOWN;
    } else {
      ev->type = SDL_KEYUP;
    }
    int len = strlen(buf);
    buf[len - 1] = '\0';
    for (int i = 0; i < keynum; i++) {
      if (strcmp(buf + 3, keyname[i]) == 0) {
        ev->key.keysym.sym = i;
        if (ev->type == SDL_KEYDOWN) {
          keystate[i] = 1;
        } else {
          keystate[i] = 0;
        }
        break;
      }
    }
    return 1;
  }
}

int SDL_PeepEvents(SDL_Event *ev, int numevents, int action, uint32_t mask) {
  return 0;
}

uint8_t* SDL_GetKeyState(int *numkeys) {
  // if (numkeys) *numkeys = keynum;
  // return keystate;
  return NULL;
}
