#include <fs.h>

typedef size_t (*ReadFn) (void *buf, size_t offset, size_t len);
typedef size_t (*WriteFn) (const void *buf, size_t offset, size_t len);

typedef struct {
  char *name;
  size_t size;
  size_t disk_offset;
  ReadFn read;
  WriteFn write;
  size_t open_offset;
} Finfo;

enum {FD_STDIN, FD_STDOUT, FD_STDERR, FD_FB};

extern size_t ramdisk_read(void *buf, size_t offset, size_t len);
extern size_t ramdisk_write(const void *buf, size_t offset, size_t len);
size_t invalid_read(void *buf, size_t offset, size_t len) {
  panic("should not reach here");
  return 0;
}

size_t invalid_write(const void *buf, size_t offset, size_t len) {
  panic("should not reach here");
  return 0;
}

extern size_t serial_write(const void *buf, size_t offset, size_t len);
extern size_t events_read(void *buf, size_t offset, size_t len);
extern size_t dispinfo_read(void *buf, size_t offset, size_t len);
extern size_t fb_write(const void *buf, size_t offset, size_t len);
/* This is the information about all files in disk. */
static Finfo file_table[] __attribute__((used)) = {
  [FD_STDIN]  = {"stdin", 0, 0, invalid_read, invalid_write},
  [FD_STDOUT] = {"stdout", 0, 0, invalid_read, serial_write},
  [FD_STDERR] = {"stderr", 0, 0, invalid_read, serial_write},
  [FD_FB] = {"/dev/fb", 0, 0, invalid_read, fb_write},
  {"/dev/events", 0, 0, events_read, invalid_write},
  {"/proc/dispinfo", 0, 0, dispinfo_read, invalid_write},
#include "files.h"
};

void init_fs() {
  int w = io_read(AM_GPU_CONFIG).width;
  int h = io_read(AM_GPU_CONFIG).height;
  file_table[FD_FB].size = w * h * sizeof(uint32_t);
}

int fs_open(const char *pathname, int flags, int mode) {
  size_t file_table_nr = sizeof(file_table) / sizeof(Finfo);
  for (int i = 0; i < file_table_nr; i++) {
    if (strcmp(pathname, file_table[i].name) == 0) {
      file_table[i].open_offset = 0;
      return i;
    }
  }
  printf("pathname: %s\n", pathname);
  panic("invalid pathname!");
  return -1;
}
size_t fs_read(int fd, void *buf, size_t len) {
  Finfo *cur = &file_table[fd];
  if (cur->read) return cur->read(buf, cur->open_offset, len);
  // assert(cur->open_offset + len <= cur->size);
  size_t read_len = cur->size - cur->open_offset < len ? cur->size - cur->open_offset : len;
  ramdisk_read(buf, cur->disk_offset + cur->open_offset, read_len);
  cur->open_offset += read_len;
  return read_len;
}
size_t fs_write(int fd, const void *buf, size_t len) {
  // yield();
  Finfo *cur = &file_table[fd];
  if (cur->write) return cur->write(buf, cur->open_offset, len);
  // assert(cur->open_offset + len <= cur->size);
  size_t write_len = cur->size - cur->open_offset < len ? cur->size - cur->open_offset : len;
  ramdisk_write(buf, cur->disk_offset + cur->open_offset, write_len);
  cur->open_offset += write_len;
  return write_len;
}
size_t fs_lseek(int fd, size_t offset, int whence) {
  assert(fd >= 2);
  Finfo *cur = &file_table[fd];
  switch (whence) {
    case SEEK_SET: cur->open_offset = offset; break;
    case SEEK_CUR: cur->open_offset += offset; break;
    case SEEK_END: cur->open_offset = cur->size + offset; break;
    default: panic("invalid whence!");
  }
  assert(cur->open_offset <= cur->size);
  return cur->open_offset;
}

int fs_close(int fd) {
  // file_table[fd].open_offset = 0;
  return 0;
}