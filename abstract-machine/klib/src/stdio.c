#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

int my_itoa(int value, char *str) {
    int count = 0;
    int negative = 0;
    if (value < 0) {
      negative = 1;
      value = -value;
    }
    do {
      str[count++] = (value % 10) + '0';
      value /= 10;
    } while (value != 0);
    if (negative) {
      str[count++] = '-';
    }
    for (int i = 0; i < count / 2; i++) {
      char tmp = str[i];
      str[i] = str[count - i - 1];
      str[count - i - 1] = tmp;
    }
    return count;
}

int my_itoa_hex(unsigned int value, char *str) {
    int count = 0;
    char hex_chars[] = "0123456789abcdef";
    if (value == 0) {
        str[count++] = '0';
    } else {
        while (value != 0) {
            str[count++] = hex_chars[value % 16];
            value /= 16;
        }
    }
    for (int i = 0; i < count / 2; i++) {
        char tmp = str[i];
        str[i] = str[count - i - 1];
        str[count - i - 1] = tmp;
    }
    return count;
}

int printf(const char *fmt, ...) {
  char buffer[4096];
  va_list ap;
  va_start(ap, fmt);
  int ret = vsprintf(buffer, fmt, ap);
  int len = strlen(buffer);
  for (int i = 0; i < len; i++) {
    putch(buffer[i]);
  }
  return ret;
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  return vsnprintf(out, -1, fmt, ap);
}

int sprintf(char *out, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  return vsnprintf(out, -1, fmt, ap);
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  return vsnprintf(out, n, fmt, ap);
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  int count = 0;
  const char *p;
  for (p = fmt; *p != '\0' && count < n - 1; p++) {
    if (*p == '%') {
      p++;
      switch (*p) {
        case 'd': {
          int num = va_arg(ap, int);
          count += my_itoa(num, out + count);
          break;
        }
        case 's': {
          const char *str = va_arg(ap, const char *);
          while (*str && count < n - 1) {
            out[count++] = *str++;
          }
          break;
        }
        case 'c': {
          char ch = (char)va_arg(ap, int);
          if (count < n - 1) {
            out[count++] = ch;
          }
          break;
        }
        case 'x': {
          out[count++] = '0';
          out[count++] = 'x';
          int num = va_arg(ap, int);
          count += my_itoa_hex(num, out + count);
          break;
        }
        case '%': 
          out[count++] = '%';
          break;
        case 'p': {
          out[count++] = '0';
          out[count++] = 'x';
          int ptr = ((int)va_arg(ap, void *));
          count += my_itoa_hex(ptr, out + count);
          break;
        }
        default: 
          out[count++] = *p;
          // printf("Unknown format specifier: %c%c%c%c\n", *p, *(p + 1), *(p + 2), *(p + 3));
          // panic("Unknown format specifier");
      }
    } else {
      if (count < n - 1) {
        out[count++] = *p;
      }
    }
  }
  out[count] = '\0';
  va_end(ap);
  return count;
}

#endif
//完成
