#define SDL_malloc  malloc
#define SDL_free    free
#define SDL_realloc realloc

#define SDL_STBIMAGE_IMPLEMENTATION
#include "SDL_stbimage.h"

SDL_Surface* IMG_Load_RW(SDL_RWops *src, int freesrc) {
  assert(src->type == RW_TYPE_MEM);
  assert(freesrc == 0);
  return NULL;
}

SDL_Surface* IMG_Load(const char *filename) {
  FILE *f = fopen(filename, "rb");
  if (f == NULL) {
    return NULL;
  }
  fseek(f, 0, SEEK_END);
  size_t size = ftell(f);
  fseek(f, 0, SEEK_SET);

  unsigned char *buffer = (unsigned char *)malloc(size);
  if (buffer == NULL) {
    fclose(f);
    return NULL;
  }

  size_t read_size = fread(buffer, 1, size, f);
  if (read_size != size) {
    free(buffer);
    fclose(f);
    return NULL;
  }

  SDL_Surface *surface = STBIMG_LoadFromMemory(buffer, size);
  if (surface == NULL) {
    free(buffer);
    return NULL;
  }
  free(buffer);
  fclose(f);
  return surface;
}

int IMG_isPNG(SDL_RWops *src) {
  return 0;
}

SDL_Surface* IMG_LoadJPG_RW(SDL_RWops *src) {
  return IMG_Load_RW(src, 0);
}

char *IMG_GetError() {
  return "Navy does not support IMG_GetError()";
}
