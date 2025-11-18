/* shell.c - Minimalistic kernel shell implementation */

#include "../include/shell.h"
#include "../include/printf.h"
#include "../include/string.h"
#include "../include/vga.h"
#include "../include/stack.h"
#include "../include/gdt.h"
#include "../include/keyboard.h"
#include "../include/io.h"
#include "../include/kmalloc.h"
#include "../include/vmalloc.h"
#include "../include/paging.h"
#include "../include/panic.h"
#include "../include/signal.h"
#include "../include/syscall.h"
#include "../include/idt.h"
#include "../include/process.h"

/* Shell state */
static char shell_buffer[SHELL_BUFFER_SIZE];
static int shell_buffer_pos = 0;

/* Command function prototypes */
static void cmd_help(int argc, char **argv);
static void cmd_clear(int argc, char **argv);
static void cmd_stack(int argc, char **argv);
static void cmd_stacktrace(int argc, char **argv);
static void cmd_gdt(int argc, char **argv);
static void cmd_reboot(int argc, char **argv);
static void cmd_halt(int argc, char **argv);
static void cmd_echo(int argc, char **argv);
static void cmd_about(int argc, char **argv);
static void cmd_mem(int argc, char **argv);
static void cmd_kmalloc_stats(int argc, char **argv);
static void cmd_vmalloc_stats(int argc, char **argv);
static void cmd_memtest(int argc, char **argv);
static void cmd_panic(int argc, char **argv);
static void cmd_signal(int argc, char **argv);
static void cmd_syscall(int argc, char **argv);
static void cmd_idt(int argc, char **argv);
static void cmd_process(int argc, char **argv);
static void cmd_fork(int argc, char **argv);
static void cmd_psignal(int argc, char **argv);
static void cmd_mmap(int argc, char **argv);

/* Command structure */
struct shell_command {
    const char *name;
    const char *description;
    void (*handler)(int argc, char **argv);
};

/* Available commands */
static const struct shell_command commands[] = {
    {"help",       "Display this help message", cmd_help},
    {"clear",      "Clear the screen", cmd_clear},
    {"stack",      "Display kernel stack information", cmd_stack},
    {"stacktrace", "Display stack trace", cmd_stacktrace},
    {"gdt",        "Display GDT information", cmd_gdt},
    {"idt",        "Display IDT information", cmd_idt},
    {"mem",        "Display memory information", cmd_mem},
    {"kstats",     "Display kernel heap statistics", cmd_kmalloc_stats},
    {"vstats",     "Display virtual memory statistics", cmd_vmalloc_stats},
    {"memtest",    "Test memory allocation", cmd_memtest},
    {"panic",      "Trigger a kernel panic", cmd_panic},
    {"signal",     "Test signal system", cmd_signal},
    {"syscall",    "Test syscall system", cmd_syscall},
    {"process",    "Test process system", cmd_process},
    {"fork",       "Test fork syscall", cmd_fork},
    {"psignal",    "Test process signal", cmd_psignal},
    {"mmap",       "Test mmap syscall", cmd_mmap},
    {"reboot",     "Reboot the system", cmd_reboot},
    {"halt",       "Halt the system", cmd_halt},
    {"echo",       "Echo arguments", cmd_echo},
    {"about",      "About this kernel", cmd_about},
    {NULL, NULL, NULL}  /* Sentinel */
};

/* Initialize shell */
void shell_init(void) {
    shell_buffer_pos = 0;
    shell_buffer[0] = '\0';
}

/* Print shell prompt */
static void shell_prompt(void) {
    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    printk("kfs");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    printk("$ ");
}

/* Parse command line into arguments */
static int shell_parse_args(char *cmdline, char **argv, int max_args) {
    int argc = 0;
    char *p = cmdline;
    bool in_arg = false;

    while (*p && argc < max_args) {
        if (*p == ' ' || *p == '\t' || *p == '\n') {
            if (in_arg) {
                *p = '\0';
                in_arg = false;
            }
        } else {
            if (!in_arg) {
                argv[argc++] = p;
                in_arg = true;
            }
        }
        p++;
    }

    return argc;
}

/* Process a command */
void shell_process_command(const char *command) {
    char cmd_copy[SHELL_BUFFER_SIZE];
    char *argv[SHELL_MAX_ARGS];
    int argc;

    /* Skip empty commands */
    if (!command || command[0] == '\0' || command[0] == '\n') {
        return;
    }

    /* Make a copy of the command */
    strncpy(cmd_copy, command, SHELL_BUFFER_SIZE - 1);
    cmd_copy[SHELL_BUFFER_SIZE - 1] = '\0';

    /* Parse arguments */
    argc = shell_parse_args(cmd_copy, argv, SHELL_MAX_ARGS);

    if (argc == 0) {
        return;
    }

    /* Find and execute command */
    for (int i = 0; commands[i].name != NULL; i++) {
        if (strcmp(argv[0], commands[i].name) == 0) {
            commands[i].handler(argc, argv);
            return;
        }
    }

    /* Command not found */
    vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    printk("Unknown command: %s\n", argv[0]);
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    printk("Type 'help' for a list of available commands.\n");
}

/* Handle keyboard input */
void shell_handle_input(int c) {
    /* Handle backspace */
    if (c == KEY_BACKSPACE || c == '\b') {
        if (shell_buffer_pos > 0) {
            shell_buffer_pos--;
            shell_buffer[shell_buffer_pos] = '\0';

            /* Move cursor back and erase character */
            size_t row, col;
            vga_get_cursor_position(&row, &col);
            if (col > 0) {
                vga_set_cursor_position(row, col - 1);
                vga_putchar(' ');
                vga_set_cursor_position(row, col - 1);
            }
        }
        return;
    }

    /* Handle enter */
    if (c == '\n') {
        vga_putchar('\n');
        shell_buffer[shell_buffer_pos] = '\0';
        shell_process_command(shell_buffer);
        shell_buffer_pos = 0;
        shell_buffer[0] = '\0';
        shell_prompt();
        return;
    }

    /* Handle printable characters */
    if (c >= 32 && c <= 126 && shell_buffer_pos < SHELL_BUFFER_SIZE - 1) {
        shell_buffer[shell_buffer_pos++] = c;
        shell_buffer[shell_buffer_pos] = '\0';
        vga_putchar(c);
    }
}

/* ===== Command Implementations ===== */

/* Help command */
static void cmd_help(int argc, char **argv) {
    (void)argc;
    (void)argv;

    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    printk("\nAvailable Commands:\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    for (int i = 0; commands[i].name != NULL; i++) {
        vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        printk("  %s", commands[i].name);

        /* Add padding for alignment */
        int len = strlen(commands[i].name);
        for (int j = len; j < 12; j++) {
            printk(" ");
        }

        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        printk(" - %s\n", commands[i].description);
    }
    printk("\n");
}

/* Clear command */
static void cmd_clear(int argc, char **argv) {
    (void)argc;
    (void)argv;
    vga_clear();
}

/* Stack command */
static void cmd_stack(int argc, char **argv) {
    (void)argc;
    (void)argv;
    stack_print();
}

/* Stack trace command */
static void cmd_stacktrace(int argc, char **argv) {
    int max_frames = 10;

    if (argc > 1) {
        /* Parse max frames from argument */
        max_frames = 0;
        const char *p = argv[1];
        while (*p >= '0' && *p <= '9') {
            max_frames = max_frames * 10 + (*p - '0');
            p++;
        }
        if (max_frames == 0) {
            max_frames = 10;
        }
    }

    stack_print_frames(max_frames);
}

/* GDT command */
static void cmd_gdt(int argc, char **argv) {
    (void)argc;
    (void)argv;
    gdt_print_info();
}

/* Reboot command */
static void cmd_reboot(int argc, char **argv) {
    (void)argc;
    (void)argv;

    vga_set_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK);
    printk("Rebooting system...\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    /* Use keyboard controller to reboot */
    uint8_t good = 0x02;
    while (good & 0x02) {
        good = inb(0x64);
    }
    outb(0x64, 0xFE);

    /* If that didn't work, try triple fault */
    __asm__ volatile("cli");
    __asm__ volatile("hlt");
}

/* Halt command */
static void cmd_halt(int argc, char **argv) {
    (void)argc;
    (void)argv;

    vga_set_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK);
    printk("System halted. You can now power off.\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    __asm__ volatile("cli");
    while (1) {
        __asm__ volatile("hlt");
    }
}

/* Echo command */
static void cmd_echo(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        printk("%s", argv[i]);
        if (i < argc - 1) {
            printk(" ");
        }
    }
    printk("\n");
}

/* About command */
static void cmd_about(int argc, char **argv) {
    (void)argc;
    (void)argv;

    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    printk("\n=== KFS_4: Interrupt System ===\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    printk("\nKernel From Scratch - Fourth Subject\n");
    printk("A minimal 32-bit x86 kernel with:\n");
    printk("  - Interrupt Descriptor Table (IDT)\n");
    printk("  - CPU Exception Handlers (0x00-0x13)\n");
    printk("  - Hardware Interrupts (PIC)\n");
    printk("  - Signal-callback system\n");
    printk("  - Syscall infrastructure (INT 0x80)\n");
    printk("  - Global Descriptor Table (GDT)\n");
    printk("  - Memory paging and allocators\n");
    printk("  - Kernel panic handling\n");
    printk("  - Interactive debug shell\n");
    printk("\nArchitecture: i386 (x86)\n");
    printk("Boot Loader: GRUB Multiboot\n");
    printk("No standard library dependencies\n");
    printk("\n");
}

/* Memory info command */
static void cmd_mem(int argc, char **argv) {
    (void)argc;
    (void)argv;

    printk("\n=== Memory System Overview ===\n");
    printk("Page size: %d bytes\n", PAGE_SIZE);
    printk("Pages per table: %d\n", PAGE_ENTRIES);
    printk("Pages per directory: %d\n", PAGE_ENTRIES);
    printk("Virtual address space: 4 GB\n");
    printk("\nMemory regions:\n");
    printk("  Kernel heap:     0x00500000 - 0x00600000 (1 MB)\n");
    printk("  Virtual memory:  0x10000000 - 0x20000000 (256 MB)\n");
    printk("\nType 'kstats' for kernel heap statistics\n");
    printk("Type 'vstats' for virtual memory statistics\n\n");
}

/* Kernel heap statistics command */
static void cmd_kmalloc_stats(int argc, char **argv) {
    (void)argc;
    (void)argv;
    kmalloc_stats();
}

/* Virtual memory statistics command */
static void cmd_vmalloc_stats(int argc, char **argv) {
    (void)argc;
    (void)argv;
    vmalloc_stats();
}

/* Memory test command */
static void cmd_memtest(int argc, char **argv) {
    (void)argc;
    (void)argv;

    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    printk("\n=== Memory Allocation Test ===\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    /* Test kmalloc */
    printk("\nTesting kmalloc...\n");
    void *ptr1 = kmalloc(1024);
    printk("  Allocated 1024 bytes at 0x%x\n", (uint32_t)ptr1);

    void *ptr2 = kmalloc(2048);
    printk("  Allocated 2048 bytes at 0x%x\n", (uint32_t)ptr2);

    void *ptr3 = kmalloc(512);
    printk("  Allocated 512 bytes at 0x%x\n", (uint32_t)ptr3);

    /* Test kfree */
    printk("\nTesting kfree...\n");
    kfree(ptr2);
    printk("  Freed 0x%x\n", (uint32_t)ptr2);

    /* Test vmalloc */
    printk("\nTesting vmalloc...\n");
    void *vptr1 = vmalloc(8192);
    printk("  Allocated 8192 bytes at 0x%x\n", (uint32_t)vptr1);

    void *vptr2 = vmalloc(16384);
    printk("  Allocated 16384 bytes at 0x%x\n", (uint32_t)vptr2);

    /* Show statistics */
    printk("\nCurrent memory state:\n");
    kmalloc_stats();
    vmalloc_stats();

    vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    printk("Memory test completed successfully!\n\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
}

/* Panic command - test kernel panic */
static void cmd_panic(int argc, char **argv) {
    (void)argc;
    (void)argv;

    vga_set_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK);
    printk("Triggering kernel panic...\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    /* Trigger a kernel panic */
    kernel_panic("Test panic from shell");
}

/* Test signal handler */
static void test_signal_handler(int sig) {
    vga_set_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK);
    printk("\n[SIGNAL] Handler called for signal %d (%s)\n", sig, signal_name(sig));
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
}

/* Signal command - test signal system */
static void cmd_signal(int argc, char **argv) {
    (void)argc;
    (void)argv;

    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    printk("\n=== Signal System Test ===\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    /* Register a signal handler */
    printk("Registering handler for SIGINT (2)...\n");
    signal_register(SIGINT, test_signal_handler);

    /* Raise the signal */
    printk("Raising SIGINT...\n");
    signal_raise(SIGINT);

    /* Process pending signals */
    printk("Processing pending signals...\n");
    signal_process_pending();

    printk("\nSignal test completed!\n\n");
}

/* Syscall command - test syscall system */
static void cmd_syscall(int argc, char **argv) {
    (void)argc;
    (void)argv;

    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    printk("\n=== Syscall System Test ===\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    printk("Testing syscalls via INT 0x80...\n\n");

    /* Test sys_write (syscall 1) */
    printk("Test 1: sys_write (SYS_WRITE = 1)\n");
    const char *msg = "  Hello from syscall!\n";
    int len = 0;
    while (msg[len]) len++;

    int result;
    __asm__ volatile(
        "mov $1, %%eax\n"      /* syscall number: SYS_WRITE */
        "mov $1, %%ebx\n"      /* fd: stdout */
        "mov %1, %%ecx\n"      /* buffer */
        "mov %2, %%edx\n"      /* count */
        "int $0x80\n"          /* syscall */
        "mov %%eax, %0\n"      /* get result */
        : "=r"(result)
        : "r"(msg), "r"(len)
        : "eax", "ebx", "ecx", "edx"
    );
    printk("  Result: %d bytes written\n\n", result);

    /* Test sys_exit (syscall 0) */
    printk("Test 2: sys_exit (SYS_EXIT = 0)\n");
    __asm__ volatile(
        "mov $0, %%eax\n"      /* syscall number: SYS_EXIT */
        "mov $42, %%ebx\n"     /* exit status */
        "int $0x80\n"          /* syscall */
        ::: "eax", "ebx"
    );

    printk("\nSyscall test completed!\n\n");
}

/* IDT command - display IDT information */
static void cmd_idt(int argc, char **argv) {
    (void)argc;
    (void)argv;

    idt_print_info();
}

/* Test process entry point */
static void test_process_entry(void) {
    printk("[PROCESS] Test process running! PID: %d\n",
           process_get_current() ? process_get_current()->pid : 0);

    /* Simulate some work */
    for (volatile int i = 0; i < 1000000; i++);

    printk("[PROCESS] Test process exiting\n");
    process_exit(process_get_current(), 42);
}

/* Process command - test process creation */
static void cmd_process(int argc, char **argv) {
    (void)argc;
    (void)argv;

    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    printk("\n=== Process System Test ===\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    printk("Creating a test process...\n");
    process_t *proc = process_create(test_process_entry, 0);

    if (proc) {
        vga_set_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK);
        printk("Process created successfully!\n");
        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        printk("  PID: %d\n", proc->pid);
        printk("  UID: %d\n", proc->uid);
        printk("  State: %d\n", proc->state);
        printk("  Kernel stack: 0x%x\n", proc->kernel_stack);
        printk("  User stack: 0x%x\n", proc->user_stack);
    } else {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        printk("Failed to create process!\n");
        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    }

    printk("\nProcess test completed!\n\n");
}

/* Fork command - test fork syscall */
static void cmd_fork(int argc, char **argv) {
    (void)argc;
    (void)argv;

    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    printk("\n=== Fork System Test ===\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    /* First create a parent process */
    printk("Creating parent process...\n");
    process_t *parent = process_create(test_process_entry, 0);

    if (!parent) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        printk("Failed to create parent process!\n");
        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        return;
    }

    printk("Parent process created (PID: %d)\n", parent->pid);

    /* Now fork it */
    printk("\nForking parent process...\n");
    process_t *child = process_fork(parent);

    if (child) {
        vga_set_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK);
        printk("Fork successful!\n");
        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        printk("  Parent PID: %d\n", parent->pid);
        printk("  Child PID:  %d\n", child->pid);
        printk("  Child parent: %d\n", child->parent ? child->parent->pid : 0);
    } else {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        printk("Fork failed!\n");
        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    }

    printk("\nFork test completed!\n\n");
}

/* Process signal handler for testing */
static void test_process_signal_handler(int sig) {
    vga_set_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK);
    printk("\n[PSIGNAL] Process received signal %d\n", sig);
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
}

/* Process signal command - test process signal handling */
static void cmd_psignal(int argc, char **argv) {
    (void)argc;
    (void)argv;

    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    printk("\n=== Process Signal Test ===\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    /* Create a test process */
    printk("Creating test process...\n");
    process_t *proc = process_create(test_process_entry, 0);

    if (!proc) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        printk("Failed to create process!\n");
        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        return;
    }

    printk("Process created (PID: %d)\n", proc->pid);

    /* Register a signal handler */
    printk("Registering signal handler for SIGINT (2)...\n");
    process_signal_register(proc, 2, test_process_signal_handler);

    /* Send a signal */
    printk("Sending SIGINT to process...\n");
    process_signal_send(proc, 2);

    /* Process pending signals */
    printk("Processing pending signals...\n");
    process_signal_process(proc);

    vga_set_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK);
    printk("\nProcess signal test completed!\n\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
}

/* MMAP command - test memory mapping */
static void cmd_mmap(int argc, char **argv) {
    (void)argc;
    (void)argv;

    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    printk("\n=== Memory Mapping (mmap) Test ===\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    /* Create a test process */
    printk("Creating test process...\n");
    process_t *proc = process_create(test_process_entry, 0);

    if (!proc) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        printk("Failed to create process!\n");
        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        return;
    }

    printk("Process created (PID: %d)\n", proc->pid);
    printk("Initial heap: 0x%x - 0x%x\n", proc->heap_start, proc->heap_end);

    /* Test mmap - allocate 8KB */
    printk("\nMapping 8KB with mmap...\n");
    void *mapped = process_mmap(proc, NULL, 8192, PROT_READ | PROT_WRITE,
                                 MAP_PRIVATE | MAP_ANONYMOUS);

    if (mapped == (void *)-1) {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        printk("mmap failed!\n");
        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        return;
    }

    vga_set_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK);
    printk("mmap successful!\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    printk("  Mapped at: 0x%x\n", (uint32_t)mapped);
    printk("  New heap end: 0x%x\n", proc->heap_end);

    /* Test brk - grow heap */
    printk("\nTesting brk (grow heap by 4KB)...\n");
    uint32_t new_brk = proc->heap_end + 4096;
    int result = process_brk(proc, (void *)new_brk);

    if (result != -1) {
        vga_set_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK);
        printk("brk successful!\n");
        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        printk("  New brk: 0x%x\n", result);
    } else {
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        printk("brk failed!\n");
        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    }

    /* Display process memory layout */
    printk("\nProcess Memory Layout:\n");
    printk("  .text:   0x%x (size: %d, flags: 0x%x)\n",
           proc->text_section.start_addr, proc->text_section.size, proc->text_section.flags);
    printk("  .rodata: 0x%x (size: %d, flags: 0x%x)\n",
           proc->rodata_section.start_addr, proc->rodata_section.size, proc->rodata_section.flags);
    printk("  .data:   0x%x (size: %d, flags: 0x%x)\n",
           proc->data_section.start_addr, proc->data_section.size, proc->data_section.flags);
    printk("  .bss:    0x%x (size: %d, flags: 0x%x)\n",
           proc->bss_section.start_addr, proc->bss_section.size, proc->bss_section.flags);
    printk("  heap:    0x%x - 0x%x\n", proc->heap_start, proc->heap_end);
    printk("  stack:   0x%x\n", proc->user_stack);

    vga_set_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK);
    printk("\nMemory mapping test completed!\n\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
}

/* Shell welcome message */
static void shell_welcome(void) {
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE);
    printk("============================================\n");
    printk("       KFS_4 - Interrupt System Shell      \n");
    printk("============================================\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    printk("\n");

    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    printk("Welcome to the KFS Debug Shell!\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    printk("Type 'help' for a list of commands.\n");
    printk("Type 'panic', 'signal', 'syscall', 'idt' to test.\n");
    printk("Press Alt+F1 to Alt+F4 to switch screens.\n\n");
}

/* Run the shell */
void shell_run(void) {
    shell_init();
    shell_welcome();
    shell_prompt();

    /* Main shell loop */
    while (1) {
        if (keyboard_haskey()) {
            int c = keyboard_getchar();

            if (c == 0) {
                continue;
            }

            /* Handle screen switching */
            if (c >= KEY_F1 && c <= KEY_F4) {
                extern void handle_screen_switch(int screen_num);
                int screen_num = c - KEY_F1;
                handle_screen_switch(screen_num);
                continue;
            }

            /* Handle shell input */
            shell_handle_input(c);
        } else {
            /* No input available - halt CPU until next interrupt (saves CPU!) */
            __asm__ volatile("hlt");
        }
    }
}
