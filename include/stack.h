/* stack.h - Stack inspection and printing utilities */

#ifndef STACK_H
#define STACK_H

#include "types.h"

/* Print the kernel stack in a human-friendly format */
void stack_print(void);

/* Get the current stack pointer */
uint32_t get_stack_pointer(void);

/* Get the current base pointer */
uint32_t get_base_pointer(void);

/* Print stack frame information */
void stack_print_frames(int max_frames);

#endif /* STACK_H */
