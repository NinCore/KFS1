/* kernel.c - Main kernel entry point with bonus features demo */

#include "../include/vga.h"
#include "../include/types.h"
#include "../include/printf.h"
#include "../include/keyboard.h"
#include "../include/screen.h"

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

/* Process keyboard input */
static void handle_keyboard(void) {
    while (1) {
        if (keyboard_haskey()) {
            int c = keyboard_getchar();

            if (c == 0) {
                continue;  /* No character or modifier key */
            }

            /* Handle screen switching (Alt+F1 through Alt+F4) */
            if (c >= KEY_F1 && c <= KEY_F4) {
                int screen_num = c - KEY_F1;
                if (screen_num < MAX_SCREENS) {
                    screen_switch(screen_num);
                }
                continue;
            }

            /* Handle backspace */
            if (c == KEY_BACKSPACE || c == '\b') {
                size_t row, col;
                vga_get_cursor_position(&row, &col);
                if (col > 2 || row > 0) {  /* Don't erase the prompt */
                    if (col == 0 && row > 0) {
                        vga_set_cursor_position(row - 1, VGA_WIDTH - 1);
                    } else if (col > 0) {
                        vga_set_cursor_position(row, col - 1);
                    }
                    vga_putchar(' ');
                    vga_get_cursor_position(&row, &col);
                    if (col > 0) {
                        vga_set_cursor_position(row, col - 1);
                    }
                }
                continue;
            }

            /* Handle regular characters */
            if (c >= 32 && c <= 126) {  /* Printable ASCII */
                vga_putchar(c);
            } else if (c == '\n') {
                vga_putchar('\n');
                printk("> ");
            }
        }

        /* Small delay to avoid consuming 100% CPU */
        for (volatile int i = 0; i < 10000; i++);
    }
}

/* Kernel main function */
void kmain(void) {
    /* Initialize all systems */
    vga_init();
    keyboard_init();
    init_screens();

    /* Enter main keyboard handling loop */
    handle_keyboard();

    /* Should never reach here */
    while (1) {
        __asm__ volatile ("hlt");
    }
}
