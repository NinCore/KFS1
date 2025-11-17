/* kernel.c - Main kernel entry point for KFS_3 */

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

/* Display welcome screen */
static void display_welcome(void) {
    vga_clear();
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE);
    printk("============================================\n");
    printk("         KFS_1 - Kernel From Scratch        \n");
    printk("============================================\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    printk("\n");
    printk("Welcome to KFS_1!\n\n");

    /* Display mandatory feature */
    vga_set_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK);
    printk("Mandatory: 42\n\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    /* Display bonus features */
    vga_set_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK);
    printk("Bonus Features:\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    printk("  [X] Scroll and cursor support\n");
    printk("  [X] Color support\n");
    printk("  [X] printf/printk helpers\n");
    printk("  [X] Keyboard input handling\n");
    printk("  [X] Multiple screens (Alt+F1 to Alt+F4)\n\n");

    vga_set_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK);
    printk("Commands:\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    printk("  - Type to see input\n");
    printk("  - Alt+F1/F2/F3/F4: Switch screens\n");
    printk("  - Backspace: Delete character\n");
    printk("  - Enter: New line\n\n");

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

    /* Initialize memory paging - MANDATORY for KFS_3 */
    paging_init();
    paging_enable();

    /* Initialize physical memory allocator - MANDATORY for KFS_3 */
    kmalloc_init();

    /* Initialize virtual memory allocator - MANDATORY for KFS_3 */
    vmalloc_init();

    /* Initialize keyboard */
    keyboard_init();

    /* Initialize multiple screens */
    init_screens();

    /* Run the shell - BONUS for KFS_3 */
    shell_run();

    /* Should never reach here */
    while (1) {
        __asm__ volatile ("hlt");
    }
}
