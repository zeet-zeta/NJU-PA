#include <am.h>
#include <nemu.h>
#include <klib.h>

#define SYNC_ADDR (VGACTL_ADDR + 4)

void __am_gpu_init() {
}

void __am_gpu_config(AM_GPU_CONFIG_T *cfg) {
  *cfg = (AM_GPU_CONFIG_T) {
    .present = true, .has_accel = false,
    .width = 0, .height = 0,
    .vmemsz = 0
  };
  uint32_t temp = inl(VGACTL_ADDR);
  cfg->width = (int)(temp >> 16);
  cfg->height = (int)(temp & 0xffff);
  cfg->vmemsz = cfg->width * cfg->height << 5;
}

// 客户程序修改帧缓存
void __am_gpu_fbdraw(AM_GPU_FBDRAW_T *ctl) {
  int x = ctl->x;
  int y = ctl->y;
  int w = ctl->w;
  int h = ctl->h;
  uint32_t *pixels = ctl->pixels;
  uint32_t *fb = (uint32_t *)(uintptr_t)FB_ADDR;
  int t = (int)(inl(VGACTL_ADDR) >> 16);
  // for (int j = y; j < y + h; j++) {
  //   for (int i = x; i < x + w; i++) {
  //     fb[t * j + i] = pixels[w * (j - y) + (i -x)];
  //   }
  // }
  for (int j = y; j < y + h; j++) {
    int fb_base = t * j + x;
    int pixel_base = (j - y) * w;
    memcpy(&fb[fb_base], &pixels[pixel_base], w * sizeof(uint32_t));
  }
  if (ctl->sync) {
    outl(SYNC_ADDR, 1);
  }
}

void __am_gpu_status(AM_GPU_STATUS_T *status) {
  status->ready = true;
}
