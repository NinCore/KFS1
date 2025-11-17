/* printf.c - Printf implementation for kernel */

#include "../include/printf.h"
#include "../include/vga.h"
#include "../include/types.h"

/* Variable arguments support */
typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_end(ap) __builtin_va_end(ap)

/* Print a single character */
static void print_char(char c) {
    vga_putchar(c);
}

/* Print a string */
static void print_string(const char *str) {
    if (!str) {
        vga_print("(null)");
        return;
    }
    while (*str) {
        print_char(*str++);
    }
}

/* Print an unsigned integer in given base */
static void print_uint(uint32_t num, int base) {
    char buffer[32];
    int i = 0;
    const char *digits = "0123456789abcdef";

    if (num == 0) {
        print_char('0');
        return;
    }

    while (num > 0) {
        buffer[i++] = digits[num % base];
        num /= base;
    }

    while (i > 0) {
        print_char(buffer[--i]);
    }
}

/* Print a signed integer */
static void print_int(int32_t num) {
    if (num < 0) {
        print_char('-');
        num = -num;
    }
    print_uint((uint32_t)num, 10);
}

/* Main printf implementation */
void printk(const char *format, ...) {
    va_list args;
    va_start(args, format);

    while (*format) {
        if (*format == '%') {
            format++;
            switch (*format) {
                case 's': {
                    const char *str = va_arg(args, const char *);
                    print_string(str);
                    break;
                }
                case 'c': {
                    char c = (char)va_arg(args, int);
                    print_char(c);
                    break;
                }
                case 'd':
                case 'i': {
                    int32_t num = va_arg(args, int32_t);
                    print_int(num);
                    break;
                }
                case 'u': {
                    uint32_t num = va_arg(args, uint32_t);
                    print_uint(num, 10);
                    break;
                }
                case 'x':
                case 'X': {
                    uint32_t num = va_arg(args, uint32_t);
                    print_string("0x");
                    print_uint(num, 16);
                    break;
                }
                case '%': {
                    print_char('%');
                    break;
                }
                default: {
                    print_char('%');
                    print_char(*format);
                    break;
                }
            }
        } else {
            print_char(*format);
        }
        format++;
    }

    va_end(args);
}
