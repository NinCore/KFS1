/* process.c - Process management implementation */

#include "../include/process.h"
#include "../include/kmalloc.h"
#include "../include/vmalloc.h"
#include "../include/string.h"
#include "../include/printf.h"
#include "../include/paging.h"

/* Process table */
struct process process_table[MAX_PROCESSES];
struct process *current_process = NULL;

/* PID bitmap for allocation */
static uint32_t pid_bitmap[MAX_PROCESSES / 32];

/* Initialize process subsystem */
void process_init(void) {
    /* Clear process table */
    memset(process_table, 0, sizeof(process_table));

    /* Mark all PIDs as free */
    memset(pid_bitmap, 0, sizeof(pid_bitmap));

    /* PID 0 is reserved for kernel/idle */
    pid_bitmap[0] |= 1;

    current_process = NULL;

    printk("[PROCESS] Process subsystem initialized\n");
}

/* Allocate a new PID */
uint32_t process_alloc_pid(void) {
    for (uint32_t i = 1; i < MAX_PROCESSES; i++) {
        uint32_t idx = i / 32;
        uint32_t bit = i % 32;

        if (!(pid_bitmap[idx] & (1 << bit))) {
            pid_bitmap[idx] |= (1 << bit);
            return i;
        }
    }
    return 0; /* No free PID */
}

/* Free a PID */
void process_free_pid(uint32_t pid) {
    if (pid == 0 || pid >= MAX_PROCESSES) {
        return;
    }

    uint32_t idx = pid / 32;
    uint32_t bit = pid % 32;
    pid_bitmap[idx] &= ~(1 << bit);
}

/* Get process by PID */
struct process *process_get_by_pid(uint32_t pid) {
    if (pid >= MAX_PROCESSES) {
        return NULL;
    }

    struct process *proc = &process_table[pid];
    if (proc->state == PROCESS_STATE_UNUSED) {
        return NULL;
    }

    return proc;
}

/* Create a new process */
struct process *process_create(void (*entry_point)(void), uint32_t uid) {
    /* Allocate PID */
    uint32_t pid = process_alloc_pid();
    if (pid == 0) {
        printk("[PROCESS] Failed to allocate PID\n");
        return NULL;
    }

    struct process *proc = &process_table[pid];

    /* Initialize process */
    memset(proc, 0, sizeof(struct process));
    proc->pid = pid;
    proc->ppid = current_process ? current_process->pid : 0;
    proc->state = PROCESS_STATE_READY;
    proc->uid = uid;
    proc->gid = uid;

    /* Initialize memory */
    if (process_memory_init(proc) != 0) {
        printk("[PROCESS] Failed to initialize process memory\n");
        process_free_pid(pid);
        return NULL;
    }

    /* Set up context */
    proc->context.eip = (uint32_t)entry_point;
    proc->context.esp = proc->memory.stack_start;
    proc->context.ebp = proc->memory.stack_start;
    proc->context.eflags = 0x202; /* IF bit set */
    proc->context.cs = 0x08;  /* Kernel code segment */
    proc->context.ds = 0x10;  /* Kernel data segment */
    proc->context.es = 0x10;
    proc->context.fs = 0x10;
    proc->context.gs = 0x10;
    proc->context.ss = 0x10;

    /* Initialize signal handlers to default */
    for (int i = 0; i < MAX_SIGNALS; i++) {
        proc->signal_handlers[i] = SIG_DFL;
    }

    /* Set parent relationship */
    if (current_process) {
        proc->parent = current_process;
        proc->sibling = current_process->children;
        current_process->children = proc;
    }

    printk("[PROCESS] Created process PID=%d\n", pid);

    return proc;
}

/* Exit current process */
void process_exit(int exit_code) {
    if (!current_process) {
        return;
    }

    printk("[PROCESS] Process %d exiting with code %d\n",
           current_process->pid, exit_code);

    /* Save exit code */
    current_process->exit_code = exit_code;

    /* Mark as zombie */
    current_process->state = PROCESS_STATE_ZOMBIE;

    /* Reparent children to init (PID 1) or kernel (PID 0) */
    struct process *child = current_process->children;
    while (child) {
        struct process *next = child->sibling;
        child->ppid = 1; /* Reparent to init */
        child->parent = process_get_by_pid(1);
        child = next;
    }

    /* Wake up parent if waiting */
    if (current_process->parent &&
        current_process->parent->state == PROCESS_STATE_BLOCKED) {
        current_process->parent->state = PROCESS_STATE_READY;
    }

    /* Schedule next process */
    scheduler_run();
}

/* Wait for child process */
int process_wait(int *status) {
    if (!current_process) {
        return -1;
    }

    /* Check for zombie children */
    struct process *child = current_process->children;
    struct process *prev = NULL;

    while (child) {
        if (child->state == PROCESS_STATE_ZOMBIE) {
            /* Found zombie child */
            int pid = child->pid;
            if (status) {
                *status = child->exit_code;
            }

            /* Remove from children list */
            if (prev) {
                prev->sibling = child->sibling;
            } else {
                current_process->children = child->sibling;
            }

            /* Clean up process */
            process_memory_free(child);
            process_free_pid(pid);
            child->state = PROCESS_STATE_UNUSED;

            return pid;
        }
        prev = child;
        child = child->sibling;
    }

    /* No zombie children, block and wait */
    current_process->state = PROCESS_STATE_BLOCKED;
    scheduler_run();

    return 0; /* Will be resumed when child exits */
}

/* Queue a signal to a process */
int process_queue_signal(struct process *proc, int signal) {
    if (!proc || signal < 0 || signal >= MAX_SIGNALS) {
        return -1;
    }

    /* Allocate signal queue entry */
    struct signal_queue_entry *entry = kmalloc(sizeof(struct signal_queue_entry));
    if (!entry) {
        return -1;
    }

    entry->signal = signal;
    entry->next = proc->signal_queue;
    proc->signal_queue = entry;

    return 0;
}

/* Deliver pending signals to process */
void process_deliver_signals(struct process *proc) {
    if (!proc) {
        return;
    }

    while (proc->signal_queue) {
        struct signal_queue_entry *entry = proc->signal_queue;
        proc->signal_queue = entry->next;

        int sig = entry->signal;
        signal_handler_t handler = proc->signal_handlers[sig];

        /* Call handler */
        if (handler != SIG_IGN && handler != SIG_DFL) {
            handler(sig);
        } else if (handler == SIG_DFL) {
            /* Default action - for now, print message */
            printk("[SIGNAL] Process %d received signal %d (%s)\n",
                   proc->pid, sig, signal_name(sig));
        }

        kfree(entry);
    }
}

/* Kill a process with a signal */
void process_kill(uint32_t pid, int signal) {
    struct process *proc = process_get_by_pid(pid);
    if (!proc) {
        printk("[PROCESS] Kill failed: PID %d not found\n", pid);
        return;
    }

    process_queue_signal(proc, signal);

    /* If process is blocked, wake it up */
    if (proc->state == PROCESS_STATE_BLOCKED) {
        proc->state = PROCESS_STATE_READY;
    }
}

/* Exec function - load and execute a function as a process */
void exec_fn(uint32_t addr, void (*function)(void), uint32_t size) {
    printk("[EXEC] Loading function at 0x%x (size=%d)\n", addr, size);

    /* Create new process */
    struct process *proc = process_create(function, 0);
    if (!proc) {
        printk("[EXEC] Failed to create process\n");
        return;
    }

    /* Copy function code to process memory */
    if (size > 0 && addr != 0) {
        memcpy((void*)proc->memory.code_start, (void*)addr, size);
        proc->memory.code_end = proc->memory.code_start + size;
    }

    /* Add to scheduler */
    scheduler_add(proc);

    printk("[EXEC] Process %d created and scheduled\n", proc->pid);
}

