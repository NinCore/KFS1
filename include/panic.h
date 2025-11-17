/* panic.h - Kernel panic handling */

#ifndef PANIC_H
#define PANIC_H

#include "types.h"

/* Saved register state structure */
struct register_state {
    uint32_t eax, ebx, ecx, edx;
    uint32_t esi, edi, ebp, esp;
    uint32_t eip, eflags;
    uint16_t cs, ds, es, fs, gs, ss;
};

/* Saved stack snapshot */
#define STACK_SAVE_SIZE 256
struct stack_snapshot {
    uint32_t data[STACK_SAVE_SIZE];
    uint32_t size;
    uint32_t esp;
    uint32_t ebp;
};

/* Kernel panic - fatal error, stops the kernel */
void kernel_panic(const char *message) __attribute__((noreturn));

/* Kernel panic with register dump */
void kernel_panic_with_registers(const char *message, struct register_state *regs) __attribute__((noreturn));

/* Kernel warning - non-fatal error, continues execution */
void kernel_warning(const char *message);

/* Kernel info - informational message */
void kernel_info(const char *message);

/* Clean/zero all general purpose registers before halt */
void registers_clean(void);

/* Save current register state */
void registers_save(struct register_state *state);

/* Restore register state */
void registers_restore(struct register_state *state);

/* Print register state */
void registers_print(struct register_state *state);

/* Save current stack snapshot */
void stack_save_snapshot(struct stack_snapshot *snapshot);

/* Print stack snapshot */
void stack_print_snapshot(struct stack_snapshot *snapshot);

#endif /* PANIC_H */
