/* process.h - Process management structures and functions */

#ifndef PROCESS_H
#define PROCESS_H

#include "types.h"
#include "paging.h"
#include "signal.h"

/* Maximum number of processes */
#define MAX_PROCESSES 256

/* Process states */
typedef enum {
    PROCESS_STATE_UNUSED = 0,    /* Slot not in use */
    PROCESS_STATE_READY,         /* Ready to run */
    PROCESS_STATE_RUNNING,       /* Currently running */
    PROCESS_STATE_BLOCKED,       /* Blocked (waiting) */
    PROCESS_STATE_ZOMBIE,        /* Terminated but not reaped */
} process_state_t;

/* Process context - saved during context switch */
struct process_context {
    uint32_t eax, ebx, ecx, edx;
    uint32_t esi, edi, ebp, esp;
    uint32_t eip;
    uint32_t eflags;
    uint32_t cs, ds, es, fs, gs, ss;
} __attribute__((packed));

/* Signal queue entry */
struct signal_queue_entry {
    int signal;
    struct signal_queue_entry *next;
};

/* Process memory layout */
struct process_memory {
    page_directory_t *page_directory;     /* Page directory for this process */
    uint32_t code_start;          /* Start of code section */
    uint32_t code_end;            /* End of code section */
    uint32_t data_start;          /* Start of data section (BONUS) */
    uint32_t data_end;            /* End of data section (BONUS) */
    uint32_t bss_start;           /* Start of BSS section (BONUS) */
    uint32_t bss_end;             /* End of BSS section (BONUS) */
    uint32_t heap_start;          /* Start of heap */
    uint32_t heap_end;            /* Current end of heap */
    uint32_t stack_start;         /* Start of stack (high address) */
    uint32_t stack_end;           /* Current stack pointer */
};

/* Process Control Block (PCB) */
struct process {
    /* Process identification */
    uint32_t pid;                 /* Process ID */
    uint32_t ppid;                /* Parent process ID */

    /* Process state */
    process_state_t state;        /* Current state */
    int exit_code;                /* Exit code (for zombies) */

    /* Process relationships */
    struct process *parent;       /* Pointer to parent process */
    struct process *children;     /* Pointer to first child */
    struct process *sibling;      /* Pointer to next sibling */

    /* Process context */
    struct process_context context; /* Saved context */

    /* Process memory */
    struct process_memory memory; /* Memory layout */

    /* Signals */
    struct signal_queue_entry *signal_queue; /* Pending signals */
    signal_handler_t signal_handlers[MAX_SIGNALS]; /* Signal handlers */

    /* Process owner */
    uint32_t uid;                 /* User ID */
    uint32_t gid;                 /* Group ID */

    /* Timing */
    uint32_t time_slice;          /* Time slice for scheduling */
    uint32_t total_time;          /* Total CPU time used */
};

/* Process table */
extern struct process process_table[MAX_PROCESSES];
extern struct process *current_process;

/* Process management functions */
void process_init(void);
struct process *process_create(void (*entry_point)(void), uint32_t uid);
int process_fork(void);
void process_exit(int exit_code);
int process_wait(int *status);
void process_kill(uint32_t pid, int signal);

/* Process scheduling */
void scheduler_init(void);
void scheduler_run(void);
struct process *scheduler_next(void);
void scheduler_add(struct process *proc);
void scheduler_remove(struct process *proc);

/* Context switching */
void context_switch(struct process *from, struct process *to);
extern void switch_to_process(struct process_context *from, struct process_context *to);

/* Process memory management */
int process_memory_init(struct process *proc);
int process_memory_copy(struct process *dest, struct process *src);
void process_memory_free(struct process *proc);
void *process_mmap(struct process *proc, void *addr, size_t length, int prot, int flags);
int process_munmap(struct process *proc, void *addr, size_t length);

/* Process utilities */
struct process *process_get_by_pid(uint32_t pid);
uint32_t process_alloc_pid(void);
void process_free_pid(uint32_t pid);

/* Signal queue management */
int process_queue_signal(struct process *proc, int signal);
void process_deliver_signals(struct process *proc);

/* Exec function for testing */
void exec_fn(uint32_t addr, void (*function)(void), uint32_t size);

#endif /* PROCESS_H */
