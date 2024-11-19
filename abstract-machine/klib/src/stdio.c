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

int my_itoa_hex(int value, char *str) {
    int count = 0;
    char hex_chars[] = "0123456789abcdef"; // 小写十六进制字符
    if (value == 0) { // 特殊情况处理
        str[count++] = '0';
    } else {
        while (value != 0) {
            str[count++] = hex_chars[value % 16]; // 获取十六进制字符
            value /= 16;
        }
    }
    
    // 反转字符串
    for (int i = 0; i < count / 2; i++) {
        char tmp = str[i];
        str[i] = str[count - i - 1];
        str[count - i - 1] = tmp;
    }
    return count;
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
    int count = 0;
    const char *p;
    for (p = fmt; *p != '\0' && count < n - 1; p++) {
        if (*p == '%') {
            p++;
            int width = 0;
            int is_zero_padded = 0;

            // 解析宽度
            if (*p == '0') {
                is_zero_padded = 1; // 设定为零填充
                p++;
            }
            while (*p >= '0' && *p <= '9') {
                width = width * 10 + (*p - '0');
                p++;
            }

            switch (*p) {
                case 'd': {
                    int num = va_arg(ap, int);
                    count += my_itoa(num, out + count);
                    break;
                }
                case 'x': { // 处理十六进制输出
                    int num = va_arg(ap, int);
                    char hex_str[8]; // 足够大的字符数组来存放十六进制数字
                    int hex_length = my_itoa_hex(num, hex_str); // 获取十六进制字符串长度
                    
                    // 进行填充
                    if (is_zero_padded && hex_length < width) {
                        while (hex_length < width) {
                            if (count < n - 1) {
                                out[count++] = '0';
                            }
                            hex_length++;
                        }
                    }

                    // 复制十六进制字符串到输出
                    for (int i = 0; i < hex_length && count < n - 1; i++) {
                        out[count++] = hex_str[i];
                    }
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
            }
        } else {
            if (count < n - 1) {
                out[count++] = *p;
            }
        }
    }
    out[count] = '\0';
    return count;
}

// 其他函数（如 printf、sprintf、snprintf）保持不变

#endif