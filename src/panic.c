/* panic.c - Kernel panic handling implementation */

#include "../include/panic.h"
#include "../include/printf.h"
#include "../include/vga.h"

/* Kernel panic - fatal error */
void kernel_panic(const char *message) {
    /* Disable interrupts */
    __asm__ volatile("cli");

    /* Clear screen and display panic message */
    vga_clear();
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_RED);
    printk("\n\n");
    printk("  *** KERNEL PANIC ***  \n");
    printk("\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    printk("A fatal error has occurred and the kernel must stop.\n\n");
    printk("Error: %s\n\n", message);
    printk("System halted.\n");

    /* Halt the system */
    while (1) {
        __asm__ volatile("hlt");
    }
}

/* Kernel warning - non-fatal error */
void kernel_warning(const char *message) {
    vga_set_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK);
    printk("[WARNING] %s\n", message);
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
}

/* Kernel info - informational message */
void kernel_info(const char *message) {
    vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    printk("[INFO] %s\n", message);
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
}
