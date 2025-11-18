/* process.h - Process management structures and functions for KFS_5 */

#ifndef PROCESS_H
#define PROCESS_H

#include "types.h"
#include "paging.h"

/* Maximum number of processes */
#define MAX_PROCESSES 256

/* Process states */
typedef enum {
    PROCESS_STATE_UNUSED = 0,
    PROCESS_STATE_RUNNING,
    PROCESS_STATE_READY,
    PROCESS_STATE_BLOCKED,
    PROCESS_STATE_ZOMBIE
} process_state_t;

/* Process context (saved registers) */
typedef struct {
    uint32_t eax, ebx, ecx, edx;
    uint32_t esi, edi;
    uint32_t ebp, esp;
    uint32_t eip;
    uint32_t eflags;
    uint32_t cs, ds, es, fs, gs, ss;
} process_context_t;

/* Signal handler type */
typedef void (*process_signal_handler_t)(int signal);

/* Process signal queue entry */
typedef struct signal_queue_entry {
    int signal;
    struct signal_queue_entry *next;
} signal_queue_entry_t;

/* Process Control Block (PCB) */
typedef struct process {
    uint32_t pid;                    /* Process ID */
    process_state_t state;           /* Process state */

    /* Parent and children */
    struct process *parent;
    struct process *children;        /* Linked list of children */
    struct process *next_sibling;

    /* Memory management */
    page_directory_t *page_directory; /* Virtual address space */
    uint32_t kernel_stack;            /* Kernel stack pointer */
    uint32_t user_stack;              /* User stack pointer */

    /* Context (saved state when not running) */
    process_context_t context;

    /* Signals */
    signal_queue_entry_t *signal_queue;
    process_signal_handler_t signal_handlers[32];

    /* User owner */
    uint32_t uid;
    uint32_t gid;

    /* Exit status (for zombies) */
    int exit_status;

} process_t;

/* Process management functions */
void process_init(void);
process_t *process_create(void (*entry_point)(void), uint32_t uid);
process_t *process_fork(process_t *parent);
void process_exit(process_t *proc, int status);
int process_wait(process_t *parent, int *status);
void process_kill(process_t *proc, int signal);

/* Process scheduling */
void process_schedule(void);
process_t *process_get_current(void);
void process_switch(process_t *next);

/* Process signal handling */
int process_signal_register(process_t *proc, int signal, process_signal_handler_t handler);
int process_signal_send(process_t *proc, int signal);
void process_signal_process(process_t *proc);

/* Helper functions */
process_t *process_get_by_pid(uint32_t pid);
uint32_t process_get_current_uid(void);

/* System calls */
int sys_fork(uint32_t unused1, uint32_t unused2, uint32_t unused3, uint32_t unused4, uint32_t unused5);
int sys_wait(uint32_t status_ptr, uint32_t unused1, uint32_t unused2, uint32_t unused3, uint32_t unused4);
int sys_getuid(uint32_t unused1, uint32_t unused2, uint32_t unused3, uint32_t unused4, uint32_t unused5);
int sys_kill(uint32_t pid, uint32_t signal, uint32_t unused1, uint32_t unused2, uint32_t unused3);

#endif /* PROCESS_H */
