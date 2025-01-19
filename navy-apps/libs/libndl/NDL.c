#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <fcntl.h>
#include <assert.h>

static int evtdev = -1;
static int fbdev = -1;
static int screen_w = 0, screen_h = 0;
static int canvas_x = 0, canvas_y = 0;

uint32_t NDL_GetTicks() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

int NDL_PollEvent(char *buf, int len) {
  if (read(evtdev, buf, len) > 0) {
    return 1;
  } else {
    return 0;
  }
}

void NDL_OpenCanvas(int *w, int *h) {
  int fd = open("/proc/dispinfo", O_RDONLY);
  char buf[64];
  read(fd, buf, sizeof(buf) - 1);
  sscanf(buf, "WIDTH: %d\nHEIGHT: %d\n", w, h);
  screen_w = *w; screen_h = *h;
  if (*w == 0 && *h == 0) {
    *w = screen_w;
    *h = screen_h;
  }
  close(fd);
  canvas_x = (screen_w - *w) / 2;
  canvas_y = (screen_h - *h) / 2;
  assert(*w > 0 && *h > 0);
  printf("Canvas size: %d x %d\n", *w, *h);
  if (getenv("NWM_APP")) {
    int fbctl = 4;
    fbdev = 5;
    screen_w = *w; screen_h = *h;
    char buf[64];
    int len = sprintf(buf, "%d %d", screen_w, screen_h);
    // let NWM resize the window and create the frame buffer
    write(fbctl, buf, len);
    while (1) {
      // 3 = evtdev
      int nread = read(3, buf, sizeof(buf) - 1);
      if (nread <= 0) continue;
      buf[nread] = '\0';
      if (strcmp(buf, "mmap ok") == 0) break;
    }
    close(fbctl);
  }
}

void NDL_DrawRect(uint32_t *pixels, int x, int y, int w, int h) {
  printf("DrawRect: %d %d %d %d\n", x, y, w, h);
  printf("Canvas: %d %d\n", canvas_x, canvas_y);
  uint32_t *pixels_start = pixels;
  for (int i = 0; i < h; i++) {
    int pos_x = x + canvas_x;
    int pos_y = y + canvas_y + i;
    assert(pos_x >= 0 && pos_x < screen_w);
    assert(pos_y >= 0 && pos_y < screen_h);
    lseek(fbdev, (pos_x + pos_y * screen_w) * 4, SEEK_SET);
    write(fbdev, pixels_start, w * 4);
    pixels_start += w;
  }
}

void NDL_OpenAudio(int freq, int channels, int samples) {
}

void NDL_CloseAudio() {
}

int NDL_PlayAudio(void *buf, int len) {
  return 0;
}

int NDL_QueryAudio() {
  return 0;
}

int NDL_Init(uint32_t flags) {
  if (getenv("NWM_APP")) {
    evtdev = 3;
  }
  evtdev = open("/dev/events", O_RDONLY);
  fbdev = open("/dev/fb", O_RDWR);
  return 0;
}

void NDL_Quit() {
}
