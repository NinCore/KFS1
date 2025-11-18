/* process_fork.c - Fork implementation */

#include "../include/process.h"
#include "../include/string.h"
#include "../include/printf.h"

/* Fork current process */
int process_fork(void) {
    if (!current_process) {
        printk("[FORK] No current process\n");
        return -1;
    }

    printk("[FORK] Forking process %d\n", current_process->pid);

    /* Allocate PID for child */
    uint32_t child_pid = process_alloc_pid();
    if (child_pid == 0) {
        printk("[FORK] Failed to allocate PID\n");
        return -1;
    }

    struct process *child = &process_table[child_pid];

    /* Copy parent process structure */
    memcpy(child, current_process, sizeof(struct process));

    /* Set child-specific values */
    child->pid = child_pid;
    child->ppid = current_process->pid;
    child->parent = current_process;
    child->children = NULL;
    child->state = PROCESS_STATE_READY;
    child->signal_queue = NULL;

    /* Copy memory */
    if (process_memory_copy(child, current_process) != 0) {
        printk("[FORK] Failed to copy memory\n");
        process_free_pid(child_pid);
        return -1;
    }

    /* Copy context (will be modified for return value) */
    child->context = current_process->context;

    /* In child, fork returns 0 */
    child->context.eax = 0;

    /* Add child to parent's children list */
    child->sibling = current_process->children;
    current_process->children = child;

    /* Add child to scheduler */
    scheduler_add(child);

    printk("[FORK] Created child process %d from parent %d\n",
           child_pid, current_process->pid);

    /* In parent, fork returns child PID */
    return child_pid;
}

/* Syscall wrapper for fork */
int sys_fork(uint32_t unused1, uint32_t unused2, uint32_t unused3, uint32_t unused4, uint32_t unused5) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    return process_fork();
}

/* Syscall wrapper for wait */
int sys_wait(uint32_t status_ptr, uint32_t unused1, uint32_t unused2, uint32_t unused3, uint32_t unused4) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4;
    return process_wait((int*)status_ptr);
}

/* Syscall wrapper for exit */
int sys_exit(uint32_t status, uint32_t unused1, uint32_t unused2, uint32_t unused3, uint32_t unused4) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4;
    process_exit((int)status);
    return 0;  /* Never reached */
}

/* Syscall wrapper for getuid */
int sys_getuid(uint32_t unused1, uint32_t unused2, uint32_t unused3, uint32_t unused4, uint32_t unused5) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    if (current_process) {
        return (int)current_process->uid;
    }
    return 0;
}

/* Syscall wrapper for signal */
int sys_signal(uint32_t signum, uint32_t handler, uint32_t unused1, uint32_t unused2, uint32_t unused3) {
    (void)unused1; (void)unused2; (void)unused3;
    if (!current_process || (int)signum < 0 || signum >= MAX_SIGNALS) {
        return -1;
    }

    current_process->signal_handlers[signum] = (signal_handler_t)handler;
    return 0;
}

/* Syscall wrapper for kill */
int sys_kill(uint32_t pid, uint32_t sig, uint32_t unused1, uint32_t unused2, uint32_t unused3) {
    (void)unused1; (void)unused2; (void)unused3;
    process_kill(pid, (int)sig);
    return 0;
}
