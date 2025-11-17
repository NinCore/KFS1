/* panic.h - Kernel panic handling */

#ifndef PANIC_H
#define PANIC_H

/* Kernel panic - fatal error, stops the kernel */
void kernel_panic(const char *message) __attribute__((noreturn));

/* Kernel warning - non-fatal error, continues execution */
void kernel_warning(const char *message);

/* Kernel info - informational message */
void kernel_info(const char *message);

#endif /* PANIC_H */
