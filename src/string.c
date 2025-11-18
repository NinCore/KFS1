/* string.c - Basic string functions implementation */

#include "../include/string.h"

size_t strlen(const char *str) {
    size_t len = 0;
    while (str[len]) {
        len++;
    }
    return len;
}

int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

char *strcpy(char *dest, const char *src) {
    char *ptr = dest;
    while (*src) {
        *dest = *src;
        dest++;
        src++;
    }
    *dest = '\0';
    return ptr;
}

char *strncpy(char *dest, const char *src, size_t n) {
    char *ptr = dest;
    size_t i;
    for (i = 0; i < n && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    for (; i < n; i++) {
        dest[i] = '\0';
    }
    return ptr;
}

void *memset(void *ptr, int value, size_t num) {
    unsigned char *p = (unsigned char*)ptr;
    while (num--) {
        *p++ = (unsigned char)value;
    }
    return ptr;
}

void *memcpy(void *dest, const void *src, size_t num) {
    unsigned char *d = (unsigned char*)dest;
    const unsigned char *s = (const unsigned char*)src;
    while (num--) {
        *d++ = *s++;
    }
    return dest;
}

int strncmp(const char *s1, const char *s2, size_t n) {
    while (n && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) {
        return 0;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

char *strchr(const char *s, int c) {
    while (*s != '\0') {
        if (*s == (char)c) {
            return (char *)s;
        }
        s++;
    }
    /* Check if we're looking for null terminator */
    if ((char)c == '\0') {
        return (char *)s;
    }
    return NULL;
}

/* Simplified snprintf - supports %s, %d, %x only */
int snprintf(char *str, size_t size, const char *format, ...) {
    if (!str || size == 0) {
        return 0;
    }

    __builtin_va_list args;
    __builtin_va_start(args, format);

    size_t written = 0;
    const char *p = format;

    while (*p && written < size - 1) {
        if (*p == '%') {
            p++;
            if (*p == 's') {
                /* String */
                const char *s = __builtin_va_arg(args, const char *);
                if (s) {
                    while (*s && written < size - 1) {
                        str[written++] = *s++;
                    }
                }
            } else if (*p == 'd') {
                /* Integer */
                int num = __builtin_va_arg(args, int);
                char buffer[12];
                int i = 0;
                bool negative = false;

                if (num < 0) {
                    negative = true;
                    num = -num;
                }

                if (num == 0) {
                    buffer[i++] = '0';
                } else {
                    while (num > 0) {
                        buffer[i++] = '0' + (num % 10);
                        num /= 10;
                    }
                }

                if (negative && written < size - 1) {
                    str[written++] = '-';
                }

                while (i > 0 && written < size - 1) {
                    str[written++] = buffer[--i];
                }
            } else if (*p == 'x') {
                /* Hexadecimal */
                unsigned int num = __builtin_va_arg(args, unsigned int);
                char buffer[9];
                int i = 0;

                if (num == 0) {
                    buffer[i++] = '0';
                } else {
                    while (num > 0) {
                        int digit = num % 16;
                        buffer[i++] = digit < 10 ? '0' + digit : 'a' + (digit - 10);
                        num /= 16;
                    }
                }

                while (i > 0 && written < size - 1) {
                    str[written++] = buffer[--i];
                }
            } else if (*p == '%') {
                str[written++] = '%';
            }
            p++;
        } else {
            str[written++] = *p++;
        }
    }

    str[written] = '\0';
    __builtin_va_end(args);

    return written;
}
