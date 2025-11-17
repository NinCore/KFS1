/* kernel.c - Main kernel entry point */

#include "../include/vga.h"
#include "../include/types.h"

/* Kernel main function - called from boot.asm */
void kmain(void) {
    /* Initialize VGA text mode */
    vga_init();

    /* Display "42" as required by the subject */
    vga_print("42");

    /* Halt - kernel is done */
    while (1) {
        __asm__ volatile ("hlt");
    }
}
