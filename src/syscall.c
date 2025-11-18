/* syscall.c - System call implementation (BONUS) */

#include "../include/syscall.h"
#include "../include/idt.h"
#include "../include/printf.h"
#include "../include/process.h"

/* Syscall handler table */
static syscall_handler_t syscall_handlers[MAX_SYSCALLS];

/* Example syscall: write */
int sys_write(uint32_t fd, uint32_t buf, uint32_t count, uint32_t unused1, uint32_t unused2) {
    (void)unused1;
    (void)unused2;

    /* For now, just print to screen if fd=1 (stdout) */
    if (fd == 1 && buf && count > 0) {
        char *str = (char *)buf;
        for (uint32_t i = 0; i < count; i++) {
            printk("%c", str[i]);
        }
        return (int)count;
    }
    return -1;
}

/* Example syscall: read */
int sys_read(uint32_t fd, uint32_t buf, uint32_t count, uint32_t unused1, uint32_t unused2) {
    (void)fd;
    (void)buf;
    (void)count;
    (void)unused1;
    (void)unused2;

    /* Not implemented yet - would need process support */
    return -1;
}

/* Note: sys_exit is now implemented in process_fork.c */

/* Syscall dispatcher - called from INT 0x80 */
void syscall_dispatcher(struct interrupt_frame *frame) {
    /* Syscall number in EAX */
    uint32_t syscall_num = frame->eax;

    /* Arguments in EBX, ECX, EDX, ESI, EDI */
    uint32_t arg1 = frame->ebx;
    uint32_t arg2 = frame->ecx;
    uint32_t arg3 = frame->edx;
    uint32_t arg4 = frame->esi;
    uint32_t arg5 = frame->edi;

    /* Check if syscall number is valid */
    if (syscall_num >= MAX_SYSCALLS || !syscall_handlers[syscall_num]) {
        printk("[SYSCALL] Invalid syscall number: %d\n", syscall_num);
        frame->eax = -1;  /* Return error */
        return;
    }

    /* Call the syscall handler */
    int result = syscall_handlers[syscall_num](arg1, arg2, arg3, arg4, arg5);

    /* Return value in EAX */
    frame->eax = result;
}

/* Initialize syscall system */
void syscall_init(void) {
    /* Clear syscall handlers */
    for (int i = 0; i < MAX_SYSCALLS; i++) {
        syscall_handlers[i] = NULL;
    }

    /* Register INT 0x80 handler */
    idt_register_handler(INT_SYSCALL, syscall_dispatcher);

    /* Register default syscalls */
    syscall_register(SYS_WRITE, sys_write);
    syscall_register(SYS_READ, sys_read);
    syscall_register(SYS_EXIT, (syscall_handler_t)sys_exit);

    /* Register process syscalls */
    syscall_register(SYS_FORK, (syscall_handler_t)sys_fork);
    syscall_register(SYS_WAIT, (syscall_handler_t)sys_wait);
    syscall_register(SYS_GETUID, (syscall_handler_t)sys_getuid);
    syscall_register(SYS_SIGNAL, (syscall_handler_t)sys_signal);
    syscall_register(SYS_KILL, (syscall_handler_t)sys_kill);

    printk("[SYSCALL] Process syscalls registered\n");
}

/* Register a syscall handler */
int syscall_register(uint32_t syscall_num, syscall_handler_t handler) {
    if (syscall_num >= MAX_SYSCALLS) {
        return -1;
    }

    syscall_handlers[syscall_num] = handler;
    return 0;
}

/* Unregister a syscall handler */
int syscall_unregister(uint32_t syscall_num) {
    if (syscall_num >= MAX_SYSCALLS) {
        return -1;
    }

    syscall_handlers[syscall_num] = NULL;
    return 0;
}
