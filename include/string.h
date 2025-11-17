/* string.h - Basic string functions */

#ifndef STRING_H
#define STRING_H

#include "types.h"

/* Get length of string */
size_t strlen(const char *str);

/* Compare two strings */
int strcmp(const char *s1, const char *s2);

/* Copy string */
char *strcpy(char *dest, const char *src);

/* Set memory to a value */
void *memset(void *ptr, int value, size_t num);

/* Copy memory */
void *memcpy(void *dest, const void *src, size_t num);

#endif /* STRING_H */
