/* process.c - Process management implementation for KFS_5 */

#include "../include/process.h"
#include "../include/printf.h"
#include "../include/string.h"
#include "../include/kmalloc.h"
#include "../include/paging.h"
#include "../include/panic.h"

/* Process table */
static process_t process_table[MAX_PROCESSES];

/* Current running process */
static process_t *current_process = NULL;

/* Next PID to allocate */
static uint32_t next_pid = 1;

/* Initialize process system */
void process_init(void) {
    /* Clear process table */
    memset(process_table, 0, sizeof(process_table));

    /* Mark all processes as unused */
    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_table[i].state = PROCESS_STATE_UNUSED;
        process_table[i].pid = 0;
    }

    kernel_info("Process system initialized");
}

/* Allocate a new PID */
static uint32_t process_alloc_pid(void) {
    return next_pid++;
}

/* Find a free process slot */
static process_t *process_alloc_slot(void) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].state == PROCESS_STATE_UNUSED) {
            return &process_table[i];
        }
    }
    return NULL;
}

/* Create a new process */
process_t *process_create(void (*entry_point)(void), uint32_t uid) {
    /* Allocate a process slot */
    process_t *proc = process_alloc_slot();
    if (!proc) {
        return NULL;  /* No free slots */
    }

    /* Initialize process */
    memset(proc, 0, sizeof(process_t));
    proc->pid = process_alloc_pid();
    proc->state = PROCESS_STATE_READY;
    proc->uid = uid;
    proc->gid = uid;

    /* Create page directory for process */
    proc->page_directory = paging_create_directory();
    if (!proc->page_directory) {
        proc->state = PROCESS_STATE_UNUSED;
        return NULL;
    }

    /* Allocate kernel stack (1 page = 4KB) */
    proc->kernel_stack = (uint32_t)kmalloc(PAGE_SIZE);
    if (!proc->kernel_stack) {
        /* Clean up */
        paging_destroy_directory(proc->page_directory);
        proc->state = PROCESS_STATE_UNUSED;
        return NULL;
    }

    /* Allocate user stack in virtual memory (at high address) */
    /* Map user stack at 0x10000000 (256MB) - this is where the error occurs! */
    uint32_t user_stack_virt = 0x10000000;
    uint32_t user_stack_phys = (uint32_t)kmalloc(PAGE_SIZE);
    if (!user_stack_phys) {
        kfree((void *)proc->kernel_stack);
        paging_destroy_directory(proc->page_directory);
        proc->state = PROCESS_STATE_UNUSED;
        return NULL;
    }

    /* Map the user stack page */
    paging_map_page_in_directory(proc->page_directory, user_stack_virt,
                                   user_stack_phys, PAGE_WRITE | PAGE_USER);

    proc->user_stack = user_stack_virt + PAGE_SIZE - 4;  /* Stack grows down */

    /* Initialize process memory sections (KFS-5 Bonus) */
    /* .text section - executable code (Linux standard: 0x08048000) */
    proc->text_section.start_addr = 0x08048000;
    proc->text_section.size = 0;  /* Will be set when loading binary */
    proc->text_section.flags = SECTION_READ | SECTION_EXEC;

    /* .rodata section - read-only data */
    proc->rodata_section.start_addr = 0x08050000;
    proc->rodata_section.size = 0;
    proc->rodata_section.flags = SECTION_READ;

    /* .data section - initialized data */
    proc->data_section.start_addr = 0x08060000;
    proc->data_section.size = 0;
    proc->data_section.flags = SECTION_READ | SECTION_WRITE;

    /* .bss section - uninitialized data */
    proc->bss_section.start_addr = 0x08070000;
    proc->bss_section.size = 0;
    proc->bss_section.flags = SECTION_READ | SECTION_WRITE;

    /* Heap - dynamic memory allocation */
    proc->heap_start = 0x08080000;
    proc->heap_end = proc->heap_start;  /* Empty heap initially */

    /* Initialize context */
    proc->context.eip = (uint32_t)entry_point;
    proc->context.esp = proc->user_stack;
    proc->context.ebp = proc->user_stack;
    proc->context.eflags = 0x202;  /* IF flag set */

    /* Initialize signal handlers to default */
    for (int i = 0; i < 32; i++) {
        proc->signal_handlers[i] = NULL;
    }
    proc->signal_queue = NULL;

    return proc;
}

/* Fork a process (copy parent) */
process_t *process_fork(process_t *parent) {
    if (!parent) {
        return NULL;
    }

    /* Allocate a process slot */
    process_t *child = process_alloc_slot();
    if (!child) {
        return NULL;
    }

    /* Copy parent process */
    memcpy(child, parent, sizeof(process_t));

    /* Give child a new PID */
    child->pid = process_alloc_pid();
    child->state = PROCESS_STATE_READY;

    /* Set parent-child relationship */
    child->parent = parent;
    child->children = NULL;
    child->next_sibling = parent->children;
    parent->children = child;

    /* Create a new page directory (copy-on-write would be better, but simple copy for now) */
    child->page_directory = paging_clone_directory(parent->page_directory);
    if (!child->page_directory) {
        child->state = PROCESS_STATE_UNUSED;
        return NULL;
    }

    /* Allocate new kernel stack */
    child->kernel_stack = (uint32_t)kmalloc(PAGE_SIZE);
    if (!child->kernel_stack) {
        paging_destroy_directory(child->page_directory);
        child->state = PROCESS_STATE_UNUSED;
        return NULL;
    }

    /* Copy kernel stack contents */
    memcpy((void *)child->kernel_stack, (void *)parent->kernel_stack, PAGE_SIZE);

    /* Child gets return value 0, parent gets child PID */
    child->context.eax = 0;

    return child;
}

/* Exit a process */
void process_exit(process_t *proc, int status) {
    if (!proc) {
        return;
    }

    proc->exit_status = status;
    proc->state = PROCESS_STATE_ZOMBIE;

    /* Free resources (but keep PCB for parent to read exit status) */
    if (proc->page_directory) {
        paging_destroy_directory(proc->page_directory);
        proc->page_directory = NULL;
    }

    if (proc->kernel_stack) {
        kfree((void *)proc->kernel_stack);
        proc->kernel_stack = 0;
    }

    /* Orphan children - reparent to init (PID 1) */
    process_t *child = proc->children;
    while (child) {
        process_t *next = child->next_sibling;
        child->parent = NULL;  /* Or reparent to init */
        child = next;
    }

    printk("[PROCESS] Process %d exited with status %d\n", proc->pid, status);
}

/* Wait for a child process */
int process_wait(process_t *parent, int *status) {
    if (!parent) {
        return -1;
    }

    /* Find a zombie child */
    process_t *child = parent->children;
    while (child) {
        if (child->state == PROCESS_STATE_ZOMBIE) {
            /* Found a zombie child */
            uint32_t pid = child->pid;
            if (status) {
                *status = child->exit_status;
            }

            /* Remove from children list */
            process_t **prev = &parent->children;
            while (*prev && *prev != child) {
                prev = &(*prev)->next_sibling;
            }
            if (*prev) {
                *prev = child->next_sibling;
            }

            /* Free the process slot */
            child->state = PROCESS_STATE_UNUSED;
            return pid;
        }
        child = child->next_sibling;
    }

    return -1;  /* No zombie children */
}

/* Send a signal to a process */
void process_kill(process_t *proc, int signal) {
    if (!proc) {
        return;
    }

    process_signal_send(proc, signal);
}

/* Get current process */
process_t *process_get_current(void) {
    return current_process;
}

/* Get process by PID */
process_t *process_get_by_pid(uint32_t pid) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].state != PROCESS_STATE_UNUSED &&
            process_table[i].pid == pid) {
            return &process_table[i];
        }
    }
    return NULL;
}

/* Get current UID */
uint32_t process_get_current_uid(void) {
    if (current_process) {
        return current_process->uid;
    }
    return 0;  /* Root */
}

/* Register a signal handler */
int process_signal_register(process_t *proc, int signal, process_signal_handler_t handler) {
    if (!proc || signal < 0 || signal >= 32) {
        return -1;
    }

    proc->signal_handlers[signal] = handler;
    return 0;
}

/* Send a signal to a process (queue it) */
int process_signal_send(process_t *proc, int signal) {
    if (!proc || signal < 0 || signal >= 32) {
        return -1;
    }

    /* Allocate signal queue entry */
    signal_queue_entry_t *entry = (signal_queue_entry_t *)kmalloc(sizeof(signal_queue_entry_t));
    if (!entry) {
        return -1;
    }

    entry->signal = signal;
    entry->next = proc->signal_queue;
    proc->signal_queue = entry;

    return 0;
}

/* Process pending signals for a process */
void process_signal_process(process_t *proc) {
    if (!proc) {
        return;
    }

    /* Process all queued signals */
    while (proc->signal_queue) {
        signal_queue_entry_t *entry = proc->signal_queue;
        proc->signal_queue = entry->next;

        int signal = entry->signal;
        kfree(entry);

        /* Call handler if registered */
        if (proc->signal_handlers[signal]) {
            proc->signal_handlers[signal](signal);
        } else {
            /* Default action */
            printk("[SIGNAL] Process %d received signal %d (no handler)\n",
                   proc->pid, signal);
        }
    }
}

/* ===== System Calls ===== */

/* sys_fork - Fork the current process */
int sys_fork(uint32_t unused1, uint32_t unused2, uint32_t unused3, uint32_t unused4, uint32_t unused5) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5;

    process_t *parent = process_get_current();
    if (!parent) {
        return -1;  /* No current process */
    }

    process_t *child = process_fork(parent);
    if (!child) {
        return -1;  /* Fork failed */
    }

    /* Parent returns child PID, child returns 0 (set in process_fork) */
    return child->pid;
}

/* sys_wait - Wait for a child process */
int sys_wait(uint32_t status_ptr, uint32_t unused1, uint32_t unused2, uint32_t unused3, uint32_t unused4) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4;

    process_t *proc = process_get_current();
    if (!proc) {
        return -1;
    }

    int *status = (int *)status_ptr;
    return process_wait(proc, status);
}

/* sys_getuid - Get current user ID */
int sys_getuid(uint32_t unused1, uint32_t unused2, uint32_t unused3, uint32_t unused4, uint32_t unused5) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5;

    return (int)process_get_current_uid();
}

/* sys_kill - Send a signal to a process */
int sys_kill(uint32_t pid, uint32_t signal, uint32_t unused1, uint32_t unused2, uint32_t unused3) {
    (void)unused1; (void)unused2; (void)unused3;

    process_t *proc = process_get_by_pid(pid);
    if (!proc) {
        return -1;
    }

    process_kill(proc, (int)signal);
    return 0;
}

/* ===== Memory Mapping Functions (KFS-5 Bonus) ===== */

/* process_mmap - Map memory for a process */
void *process_mmap(process_t *proc, void *addr, size_t length, int prot, int flags) {
    if (!proc || length == 0) {
        return (void *)-1;
    }

    /* Round up length to page boundary */
    size_t pages_needed = (length + PAGE_SIZE - 1) / PAGE_SIZE;
    size_t total_size = pages_needed * PAGE_SIZE;

    /* Determine virtual address to use */
    uint32_t virt_addr;
    if (addr) {
        /* User specified address - align to page boundary */
        virt_addr = (uint32_t)addr & ~(PAGE_SIZE - 1);
    } else {
        /* Allocate from heap area */
        virt_addr = proc->heap_end;
    }

    /* Convert protection flags to page flags */
    uint32_t page_flags = PAGE_USER;
    if (prot & PROT_WRITE) {
        page_flags |= PAGE_WRITE;
    }

    /* Allocate and map physical pages */
    for (size_t i = 0; i < pages_needed; i++) {
        uint32_t page_virt = virt_addr + (i * PAGE_SIZE);

        /* Allocate physical page */
        void *phys_page = kmalloc(PAGE_SIZE);
        if (!phys_page) {
            /* Clean up already mapped pages */
            for (size_t j = 0; j < i; j++) {
                uint32_t cleanup_virt = virt_addr + (j * PAGE_SIZE);
                uint32_t cleanup_phys = paging_get_physical_address(cleanup_virt);
                if (cleanup_phys) {
                    kfree((void *)cleanup_phys);
                }
                paging_unmap_page(cleanup_virt);
            }
            return (void *)-1;
        }

        /* Clear the page if anonymous */
        if (flags & MAP_ANONYMOUS) {
            memset(phys_page, 0, PAGE_SIZE);
        }

        /* Map the page in process address space */
        paging_map_page_in_directory(proc->page_directory, page_virt,
                                       (uint32_t)phys_page, page_flags);
    }

    /* Update heap_end if we allocated past it */
    if (virt_addr + total_size > proc->heap_end) {
        proc->heap_end = virt_addr + total_size;
    }

    printk("[MMAP] Mapped %d bytes at 0x%x for PID %d\n", total_size, virt_addr, proc->pid);
    return (void *)virt_addr;
}

/* process_munmap - Unmap memory for a process */
int process_munmap(process_t *proc, void *addr, size_t length) {
    if (!proc || !addr || length == 0) {
        return -1;
    }

    uint32_t virt_addr = (uint32_t)addr & ~(PAGE_SIZE - 1);
    size_t pages = (length + PAGE_SIZE - 1) / PAGE_SIZE;

    /* Unmap each page */
    for (size_t i = 0; i < pages; i++) {
        uint32_t page_virt = virt_addr + (i * PAGE_SIZE);
        uint32_t page_phys = paging_get_physical_address(page_virt);

        if (page_phys) {
            /* Free physical page */
            kfree((void *)page_phys);
            /* Unmap from virtual address space */
            paging_unmap_page(page_virt);
        }
    }

    printk("[MUNMAP] Unmapped %d bytes at 0x%x for PID %d\n",
           pages * PAGE_SIZE, virt_addr, proc->pid);
    return 0;
}

/* process_brk - Change process heap size (sbrk/brk syscall) */
int process_brk(process_t *proc, void *addr) {
    if (!proc) {
        return -1;
    }

    uint32_t new_brk = (uint32_t)addr;

    /* If addr is NULL, just return current brk */
    if (!addr) {
        return (int)proc->heap_end;
    }

    /* Align to page boundary */
    new_brk = (new_brk + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

    /* Check if we're growing or shrinking the heap */
    if (new_brk > proc->heap_end) {
        /* Growing heap - allocate more pages */
        size_t size_to_add = new_brk - proc->heap_end;
        void *result = process_mmap(proc, (void *)proc->heap_end, size_to_add,
                                     PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS);
        if (result == (void *)-1) {
            return -1;
        }
    } else if (new_brk < proc->heap_end) {
        /* Shrinking heap - free pages */
        size_t size_to_remove = proc->heap_end - new_brk;
        process_munmap(proc, (void *)new_brk, size_to_remove);
        proc->heap_end = new_brk;
    }

    return (int)proc->heap_end;
}

/* sys_mmap - System call for mmap */
int sys_mmap(uint32_t addr, uint32_t length, uint32_t prot, uint32_t flags, uint32_t unused) {
    (void)unused;

    process_t *proc = process_get_current();
    if (!proc) {
        return -1;
    }

    void *result = process_mmap(proc, (void *)addr, length, (int)prot, (int)flags);
    return (int)result;
}

/* sys_brk - System call for brk */
int sys_brk(uint32_t addr, uint32_t unused1, uint32_t unused2, uint32_t unused3, uint32_t unused4) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4;

    process_t *proc = process_get_current();
    if (!proc) {
        return -1;
    }

    return process_brk(proc, (void *)addr);
}

/* ===== Exception to Signal Mapping (KFS-5 Bonus) ===== */

/* Exception to signal mapping table */
static const struct {
    uint32_t exception;
    int signal;
    const char *name;
} exception_to_signal_map[] = {
    {0,  8,  "Division by Zero → SIGFPE"},      /* #DE */
    {1,  5,  "Debug → SIGTRAP"},                /* #DB */
    {3,  5,  "Breakpoint → SIGTRAP"},           /* #BP */
    {4,  8,  "Overflow → SIGFPE"},              /* #OF */
    {5,  11, "Bound Range → SIGSEGV"},          /* #BR */
    {6,  4,  "Invalid Opcode → SIGILL"},        /* #UD */
    {7,  8,  "Device Not Available → SIGFPE"},  /* #NM */
    {8,  6,  "Double Fault → SIGABRT"},         /* #DF */
    {10, 11, "Invalid TSS → SIGSEGV"},          /* #TS */
    {11, 11, "Segment Not Present → SIGSEGV"},  /* #NP */
    {12, 11, "Stack Fault → SIGSEGV"},          /* #SS */
    {13, 11, "General Protection → SIGSEGV"},   /* #GP */
    {14, 11, "Page Fault → SIGSEGV"},           /* #PF */
    {16, 8,  "x87 FPU Error → SIGFPE"},         /* #MF */
    {17, 7,  "Alignment Check → SIGBUS"},       /* #AC */
    {18, 6,  "Machine Check → SIGABRT"},        /* #MC */
    {19, 8,  "SIMD Exception → SIGFPE"},        /* #XM */
};

#define EXCEPTION_MAP_SIZE (sizeof(exception_to_signal_map) / sizeof(exception_to_signal_map[0]))

/* Handle CPU exception by sending signal to current process (KFS-5 Bonus) */
void process_handle_exception(uint32_t exception_num) {
    process_t *proc = process_get_current();

    /* If no current process, just return (kernel-mode exception) */
    if (!proc) {
        printk("[EXCEPTION] No current process, exception %d in kernel mode\n", exception_num);
        return;
    }

    /* Find the corresponding signal */
    int signal = -1;
    for (size_t i = 0; i < EXCEPTION_MAP_SIZE; i++) {
        if (exception_to_signal_map[i].exception == exception_num) {
            signal = exception_to_signal_map[i].signal;
            printk("[IDT→PROCESS] Exception %d (%s) sending signal %d to PID %d\n",
                   exception_num, exception_to_signal_map[i].name, signal, proc->pid);
            break;
        }
    }

    /* Send signal to process if mapping found */
    if (signal != -1) {
        process_signal_send(proc, signal);
        /* Process signals immediately for critical faults */
        process_signal_process(proc);
    } else {
        printk("[EXCEPTION] No signal mapping for exception %d\n", exception_num);
    }
}
