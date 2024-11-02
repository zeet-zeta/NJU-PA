#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) {
  const char *t = s;
  while (*t) t++;
  return t - s;
}

char *strcpy(char *dst, const char *src) {
  char *original_dst = dst;
  while (*src) {
    *dst++ = *src++;
  }
  *dst = '\0';
  return original_dst;
}

char *strncpy(char *dst, const char *src, size_t n) {
  char *original_dst = dst;
  while (n > 0 && *src) {
    *dst++ = *src++;
    n--;
  }
  while (n > 0)
  {
    *dst++ = '\0';
    n--;
  }
  return original_dst;
}

char *strcat(char *dst, const char *src) {
  char *original_dst = dst;
  while (*dst) dst++;
  while (*src) {
    *dst++ = *src++;
  }
  *dst = '\0';
  return original_dst;
}

int strcmp(const char *s1, const char *s2) {
  while (*s1 && (*s1 == *s2)) {
    s1++;
    s2++;
  }
  return *(unsigned char *)s1 - *(unsigned char *)s2;
}

int strncmp(const char *s1, const char *s2, size_t n) {
  while (n > 0 && *s1 && *s2) {
    if (*s1 != *s2) {
      return *(unsigned char *)s1 - *(unsigned char *)s2;
    }
    s1++;
    s2++;
    n--;
  }
  return (n > 0) ? *(unsigned char *)s1 - *(unsigned char *)s2 : 0;
}

void *memset(void *s, int c, size_t n) {
  panic("Not implemented");
}

void *memmove(void *dst, const void *src, size_t n) {
  panic("Not implemented");
}

void *memcpy(void *out, const void *in, size_t n) {
  panic("Not implemented");
}

int memcmp(const void *s1, const void *s2, size_t n) {
  panic("Not implemented");
}

#endif
