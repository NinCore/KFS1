/* shell.c - Minimalistic kernel shell implementation */

#include "../include/shell.h"
#include "../include/printf.h"
#include "../include/string.h"
#include "../include/vga.h"
#include "../include/stack.h"
#include "../include/gdt.h"
#include "../include/keyboard.h"
#include "../include/io.h"

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
        printk("  %-12s", commands[i].name);
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
    printk("\n=== KFS_2: GDT & Stack ===\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    printk("\nKernel From Scratch - Second Subject\n");
    printk("A minimal 32-bit x86 kernel with:\n");
    printk("  - Global Descriptor Table (GDT)\n");
    printk("  - Stack management and inspection\n");
    printk("  - Minimalistic debug shell\n");
    printk("  - Keyboard input handling\n");
    printk("  - VGA text mode output\n");
    printk("\nArchitecture: i386 (x86)\n");
    printk("Boot Loader: GRUB Multiboot\n");
    printk("No standard library dependencies\n");
    printk("\n");
}

/* Shell welcome message */
static void shell_welcome(void) {
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE);
    printk("============================================\n");
    printk("        KFS_2 - GDT & Stack Shell          \n");
    printk("============================================\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    printk("\n");

    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    printk("Welcome to the KFS Debug Shell!\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    printk("Type 'help' for a list of commands.\n");
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
        }

        /* Small delay */
        for (volatile int i = 0; i < 10000; i++);
    }
}
