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

/* Process memory sections (ELF-like) */
typedef struct {
    uint32_t start_addr;             /* Start virtual address */
    uint32_t size;                   /* Section size in bytes */
    uint32_t flags;                  /* Permission flags (R/W/X) */
} process_section_t;

/* Section permission flags */
#define SECTION_READ    0x1
#define SECTION_WRITE   0x2
#define SECTION_EXEC    0x4

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

    /* Process memory sections (KFS-5 Bonus) */
    process_section_t text_section;  /* .text - executable code */
    process_section_t data_section;  /* .data - initialized data */
    process_section_t bss_section;   /* .bss - uninitialized data */
    process_section_t rodata_section;/* .rodata - read-only data */
    uint32_t heap_start;             /* Heap start address */
    uint32_t heap_end;               /* Current heap end (brk) */

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

    /* Current working directory (KFS-6 MANDATORY) */
    char pwd[256];  /* Current working directory path */

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
void process_set_current(process_t *proc);
void process_switch(process_t *next);

/* Process signal handling */
int process_signal_register(process_t *proc, int signal, process_signal_handler_t handler);
int process_signal_send(process_t *proc, int signal);
void process_signal_process(process_t *proc);

/* Exception to signal mapping (KFS-5 Bonus) */
void process_handle_exception(uint32_t exception_num);

/* Helper functions */
process_t *process_get_by_pid(uint32_t pid);
uint32_t process_get_current_uid(void);

/* Working directory functions (KFS-6 MANDATORY) */
const char *process_get_pwd(process_t *proc);
int process_set_pwd(process_t *proc, const char *path);

/* Memory mapping functions (KFS-5 Bonus) */
#define PROT_NONE   0x0  /* No access */
#define PROT_READ   0x1  /* Read access */
#define PROT_WRITE  0x2  /* Write access */
#define PROT_EXEC   0x4  /* Execute access */

#define MAP_PRIVATE 0x02 /* Private mapping */
#define MAP_ANONYMOUS 0x20 /* Anonymous mapping (no file) */

void *process_mmap(process_t *proc, void *addr, size_t length, int prot, int flags);
int process_munmap(process_t *proc, void *addr, size_t length);
int process_brk(process_t *proc, void *addr);  /* Change heap size */

/* System calls */
int sys_fork(uint32_t unused1, uint32_t unused2, uint32_t unused3, uint32_t unused4, uint32_t unused5);
int sys_wait(uint32_t status_ptr, uint32_t unused1, uint32_t unused2, uint32_t unused3, uint32_t unused4);
int sys_getuid(uint32_t unused1, uint32_t unused2, uint32_t unused3, uint32_t unused4, uint32_t unused5);
int sys_kill(uint32_t pid, uint32_t signal, uint32_t unused1, uint32_t unused2, uint32_t unused3);
int sys_mmap(uint32_t addr, uint32_t length, uint32_t prot, uint32_t flags, uint32_t unused);
int sys_brk(uint32_t addr, uint32_t unused1, uint32_t unused2, uint32_t unused3, uint32_t unused4);

#endif /* PROCESS_H */
