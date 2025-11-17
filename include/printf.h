/* printf.h - Printf and printk functions for kernel debugging */

#ifndef PRINTF_H
#define PRINTF_H

/* Print formatted string (kernel print) */
void printk(const char *format, ...);

/* Alias for printk */
#define printf printk

#endif /* PRINTF_H */
