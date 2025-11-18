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

/* Helper function to save interrupt frame to process context */
static void save_context_from_frame(struct process_context *ctx, struct interrupt_frame *frame) {
    /* Save general purpose registers */
    ctx->eax = frame->eax;
    ctx->ebx = frame->ebx;
    ctx->ecx = frame->ecx;
    ctx->edx = frame->edx;
    ctx->esi = frame->esi;
    ctx->edi = frame->edi;
    ctx->ebp = frame->ebp;

    /* Save instruction pointer and flags */
    ctx->eip = frame->eip;
    ctx->eflags = frame->eflags;

    /* Save segment registers (convert from uint32_t to uint16_t) */
    ctx->cs = (uint16_t)(frame->cs & 0xFFFF);
    ctx->ds = (uint16_t)(frame->ds & 0xFFFF);
    ctx->es = (uint16_t)(frame->es & 0xFFFF);
    ctx->fs = (uint16_t)(frame->fs & 0xFFFF);
    ctx->gs = (uint16_t)(frame->gs & 0xFFFF);

    /* CRITICAL: Handle ESP and SS correctly based on privilege level */
    /* If privilege didn't change (CS & 3 == 0), user_esp/user_ss weren't pushed */
    if ((frame->cs & 3) == 0) {
        /* Same privilege (ring 0) - calculate ESP before interrupt */
        /* Total bytes pushed: 12 (CPU: EFLAGS,CS,EIP) + 8 (ISR: err,int_no) + 16 (segments) + 32 (pushal) = 68 */
        ctx->esp = (uint32_t)frame + 68;
        ctx->ss = (uint16_t)(frame->ds & 0xFFFF); /* Use current DS as SS */

        printk("[SYSCALL] Calculated ESP = 0x%x (frame=0x%x + 68)\n", ctx->esp, (uint32_t)frame);
    } else {
        /* Privilege changed (ring 3 to ring 0) - use saved values */
        ctx->esp = frame->user_esp;
        ctx->ss = (uint16_t)(frame->user_ss & 0xFFFF);

        printk("[SYSCALL] Using user_esp = 0x%x\n", ctx->esp);
    }
}

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

    /* CRITICAL: Save current context from interrupt frame to process context */
    /* This is essential for fork() and other context-dependent syscalls */
    if (current_process) {
        save_context_from_frame(&current_process->context, frame);

        printk("[SYSCALL] Saved context for PID %d: EIP=0x%x ESP=0x%x EBP=0x%x\n",
               current_process->pid, current_process->context.eip,
               current_process->context.esp, current_process->context.ebp);
        printk("[SYSCALL] Segments: CS=0x%x DS=0x%x SS=0x%x\n",
               current_process->context.cs, current_process->context.ds,
               current_process->context.ss);
    }

    /* Call the syscall handler */
    int result = syscall_handlers[syscall_num](arg1, arg2, arg3, arg4, arg5);

    /* Return value in EAX - update BOTH frame and saved context */
    frame->eax = result;

    /* CRITICAL: Also update the saved context so if process is suspended,
     * it will resume with the correct return value */
    if (current_process) {
        current_process->context.eax = result;
        printk("[SYSCALL] Updated return value: EAX=%d\n", result);
    }
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
