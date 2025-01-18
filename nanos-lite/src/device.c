#include <common.h>

#if defined(MULTIPROGRAM) && !defined(TIME_SHARING)
# define MULTIPROGRAM_YIELD() yield()
#else
# define MULTIPROGRAM_YIELD()
#endif

#define NAME(key) \
  [AM_KEY_##key] = #key,

static const char *keyname[256] __attribute__((used)) = {
  [AM_KEY_NONE] = "NONE",
  AM_KEYS(NAME)
};

size_t serial_write(const void *buf, size_t offset, size_t len) {
  for (size_t i = 0; i < len; i++) {
    putch(((char *)buf)[i]);
  }
  return len;
}

size_t events_read(void *buf, size_t offset, size_t len) {
  bool keydown = io_read(AM_INPUT_KEYBRD).keydown;
  int keycode = io_read(AM_INPUT_KEYBRD).keycode;
  if (keycode == AM_KEY_NONE) {
    return 0;
  }
  int ret;
  if (keydown) {
    ret = snprintf(buf, len, "kd %s\n", keyname[keycode]);
  } else {
    ret = snprintf(buf, len, "ku %s\n", keyname[keycode]);
  }
  return ret;
}

size_t dispinfo_read(void *buf, size_t offset, size_t len) {
  int w = io_read(AM_GPU_CONFIG).width;
  int h = io_read(AM_GPU_CONFIG).height;
  return snprintf(buf, len, "WIDTH: %d\nHEIGHT: %d\n", w, h);
}

size_t fb_write(const void *buf, size_t offset, size_t len) {
  return 0;
}

void init_device() {
  Log("Initializing devices...");
  ioe_init();
}
