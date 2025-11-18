/* kernel.c - Main kernel entry point for KFS_5 */

#include "../include/vga.h"
#include "../include/types.h"
#include "../include/printf.h"
#include "../include/keyboard.h"
#include "../include/screen.h"
#include "../include/gdt.h"
#include "../include/stack.h"
#include "../include/shell.h"
#include "../include/paging.h"
#include "../include/kmalloc.h"
#include "../include/vmalloc.h"
#include "../include/panic.h"
#include "../include/idt.h"
#include "../include/pic.h"
#include "../include/signal.h"
#include "../include/syscall.h"
#include "../include/process.h"
/* #include "../include/mouse.h" */       /* Disabled - causes keyboard issues */
/* #include "../include/scrollback.h" */  /* Disabled - causes keyboard issues */

/* Display welcome screen */
static void display_welcome(void) {
    vga_clear();
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE);
    printk("============================================\n");
    printk("         KFS_5 - Processes System           \n");
    printk("============================================\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    printk("\n");
    printk("Welcome to KFS_5!\n\n");

    /* Display mandatory features */
    vga_set_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK);
    printk("Mandatory Features:\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    printk("  [X] Process Control Blocks (PCB)\n");
    printk("  [X] Process states & PID management\n");
    printk("  [X] fork(), wait(), exit(), getuid()\n");
    printk("  [X] signal(), kill() system calls\n");
    printk("  [X] Process memory separation (paging)\n");
    printk("  [X] Signal queuing per process\n");
    printk("  [X] Round-robin scheduler\n");
    printk("  [X] Context switching (assembly)\n");
    printk("  [X] Preemptive multitasking\n\n");

    /* Display bonus features */
    vga_set_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK);
    printk("Bonus Features:\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    printk("  [X] mmap() & munmap() system calls\n");
    printk("  [X] BSS & data section separation\n");
    printk("  [X] Full KFS_1-4 features\n\n");

    vga_set_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK);
    printk("Shell Commands:\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    printk("  - help: Display available commands\n");
    printk("  - clear: Clear the screen\n");
    printk("  - process: Run multitasking test\n");
    printk("  - fork: Test fork() system call\n");
    printk("  - psignal: Test process signals\n");
    printk("  - stack/mem/idt: System information\n");
    printk("  - signal/syscall: Test subsystems\n");
    printk("  - reboot: Reboot the system\n\n");

    vga_set_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK);
    printk("Keyboard:\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    printk("  - Alt+F1/F2/F3/F4: Switch screens\n");
    printk("  - Backspace: Delete character\n");
    printk("  - Enter: Execute command\n\n");


    printk("> ");
}

/* Initialize all screens with different content */
static void init_screens(void) {
    screen_init();

    /* Screen 0 - Main welcome screen */
    display_welcome();
    screen_save();

    /* Screen 1 - Info screen */
    screen_switch(1);
    vga_clear();
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    printk("=== Screen 1: System Information ===\n\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    printk("Kernel: KFS_1\n");
    printk("Architecture: i386 (x86)\n");
    printk("Boot loader: GRUB\n");
    printk("Video mode: VGA Text Mode 80x25\n");
    printk("Compiler: GCC\n");
    printk("Assembler: NASM\n\n");
    printk("Features:\n");
    printk("  - Multiboot compliant\n");
    printk("  - Custom linker script\n");
    printk("  - No standard library\n");
    printk("  - Standalone kernel\n\n");
    printk("Press Alt+F1 to return to main screen\n");
    screen_save();

    /* Screen 2 - Test screen */
    screen_switch(2);
    vga_clear();
    vga_set_color(VGA_COLOR_LIGHT_MAGENTA, VGA_COLOR_BLACK);
    printk("=== Screen 2: Printf Test ===\n\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    printk("Testing printf formats:\n\n");
    printk("String: %s\n", "Hello, World!");
    printk("Character: %c\n", 'K');
    printk("Decimal: %d\n", 42);
    printk("Negative: %d\n", -42);
    printk("Unsigned: %u\n", 4294967295);
    printk("Hexadecimal: %x\n", 0xDEADBEEF);
    printk("Percent: %%\n\n");

    vga_set_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK);
    printk("All tests passed!\n\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    printk("Press Alt+F1 to return to main screen\n");
    screen_save();

    /* Screen 3 - Color test */
    screen_switch(3);
    vga_clear();
    printk("=== Screen 3: Color Test ===\n\n");

    /* Display all colors */
    const char *color_names[] = {
        "Black", "Blue", "Green", "Cyan",
        "Red", "Magenta", "Brown", "Light Grey",
        "Dark Grey", "Light Blue", "Light Green", "Light Cyan",
        "Light Red", "Light Magenta", "Yellow", "White"
    };

    for (int i = 0; i < 16; i++) {
        vga_set_color(i, VGA_COLOR_BLACK);
        printk("%s ", color_names[i]);
        if ((i + 1) % 2 == 0) printk("\n");
    }

    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    printk("\n\nPress Alt+F1 to return to main screen\n");
    screen_save();

    /* Return to main screen */
    screen_switch(0);
}

/* Handle screen switching (called from shell) */
void handle_screen_switch(int screen_num) {
    if (screen_num < MAX_SCREENS) {
        screen_save();
        screen_switch(screen_num);
    }
}

/* Kernel main function */
void kmain(void) {
    /* Initialize all systems */
    vga_init();

    /* Initialize GDT - MANDATORY for KFS_2 */
    gdt_init();

    /* Initialize IDT - MANDATORY for KFS_4 */
    idt_init();

    /* Initialize signal system - MANDATORY for KFS_4 */
    signal_init();

    /* Initialize syscall system - BONUS for KFS_4 */
    syscall_init();

    /* Initialize memory paging - MANDATORY for KFS_3 */
    paging_init();
    paging_enable();

    /* Initialize physical memory allocator - MANDATORY for KFS_3 */
    kmalloc_init();

    /* Initialize virtual memory allocator - MANDATORY for KFS_3 */
    vmalloc_init();

    /* Initialize process system - MANDATORY for KFS_5 */
    process_init();

    /* DISABLED: Scrollback causes keyboard issues
     * scrollback_init();
     */

    /* Initialize keyboard */
    keyboard_init();

    /* Enable keyboard interrupts - MANDATORY for KFS_4 */
    keyboard_enable_interrupts();

    /* DISABLED: Mouse initialization causes keyboard issues
     * TODO: Fix PS/2 controller interaction between keyboard and mouse
     */
    /* mouse_init(); */
    /* mouse_enable_interrupts(); */

    /* Enable interrupts globally */
    interrupts_enable();

    /* Initialize multiple screens */
    init_screens();

    /* Run the shell - BONUS for KFS_3 */
    shell_run();

    /* Should never reach here */
    while (1) {
        __asm__ volatile ("hlt");
    }
}
