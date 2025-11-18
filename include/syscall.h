/* syscall.h - System call interface (BONUS) */

#ifndef SYSCALL_H
#define SYSCALL_H

#include "types.h"
#include "idt.h"

/* Syscall numbers */
#define SYS_EXIT    0
#define SYS_WRITE   1
#define SYS_READ    2
#define SYS_OPEN    3
#define SYS_CLOSE   4
#define SYS_GETPID  5
#define SYS_SLEEP   6
#define SYS_SIGNAL  7
#define SYS_KILL    8

#define MAX_SYSCALLS 256

/* Syscall handler function type */
typedef int (*syscall_handler_t)(uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5);

/* Initialize syscall system */
void syscall_init(void);

/* Register a syscall handler */
int syscall_register(uint32_t syscall_num, syscall_handler_t handler);

/* Unregister a syscall handler */
int syscall_unregister(uint32_t syscall_num);

/* Syscall dispatcher (called from INT 0x80) */
void syscall_dispatcher(struct interrupt_frame *frame);

/* Example syscall implementations */
int sys_write(uint32_t fd, uint32_t buf, uint32_t count, uint32_t unused1, uint32_t unused2);
int sys_read(uint32_t fd, uint32_t buf, uint32_t count, uint32_t unused1, uint32_t unused2);
int sys_exit(uint32_t status, uint32_t unused1, uint32_t unused2, uint32_t unused3, uint32_t unused4);

#endif /* SYSCALL_H */
