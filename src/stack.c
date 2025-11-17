/* stack.c - Stack inspection and printing utilities */

#include "../include/stack.h"
#include "../include/printf.h"
#include "../include/vga.h"

/* External symbols from linker */
extern uint32_t stack_bottom;
extern uint32_t stack_top;

/* Get current stack pointer */
uint32_t get_stack_pointer(void) {
    uint32_t esp;
    __asm__ volatile("mov %%esp, %0" : "=r"(esp));
    return esp;
}

/* Get current base pointer */
uint32_t get_base_pointer(void) {
    uint32_t ebp;
    __asm__ volatile("mov %%ebp, %0" : "=r"(ebp));
    return ebp;
}

/* Print the kernel stack */
void stack_print(void) {
    uint32_t esp = get_stack_pointer();
    uint32_t ebp = get_base_pointer();
    uint32_t stack_start = (uint32_t)&stack_bottom;
    uint32_t stack_end = (uint32_t)&stack_top;

    printk("\n=== Kernel Stack Information ===\n");
    printk("Stack Bottom (Start): 0x%x\n", stack_start);
    printk("Stack Top (End):      0x%x\n", stack_end);
    printk("Stack Size:           %d bytes\n", stack_end - stack_start);
    printk("Current ESP:          0x%x\n", esp);
    printk("Current EBP:          0x%x\n", ebp);
    printk("Stack Used:           %d bytes\n", stack_end - esp);
    printk("Stack Free:           %d bytes\n", esp - stack_start);

    /* Calculate stack usage percentage */
    uint32_t total_size = stack_end - stack_start;
    uint32_t used_size = stack_end - esp;
    uint32_t percentage = (used_size * 100) / total_size;

    printk("Stack Usage:          %d%%\n", percentage);

    /* Visual representation */
    printk("\nStack Growth (grows downward):\n");
    printk("  High Memory [0x%x] <-- Stack Top\n", stack_end);
    printk("               |\n");
    printk("               | Free Space\n");
    printk("               |\n");
    printk("  ESP -------> [0x%x] <-- Current Position\n", esp);
    printk("               |\n");
    printk("               | Used Space\n");
    printk("               |\n");
    printk("  Low Memory  [0x%x] <-- Stack Bottom\n", stack_start);
    printk("\n");
}

/* Print stack frames for debugging */
void stack_print_frames(int max_frames) {
    uint32_t *ebp = (uint32_t *)get_base_pointer();
    uint32_t stack_start = (uint32_t)&stack_bottom;
    uint32_t stack_end = (uint32_t)&stack_top;
    int frame = 0;

    printk("\n=== Stack Trace ===\n");

    while (ebp && frame < max_frames) {
        uint32_t ebp_addr = (uint32_t)ebp;

        /* Check if EBP is within stack bounds */
        if (ebp_addr < stack_start || ebp_addr >= stack_end) {
            printk("Frame %d: Invalid EBP (0x%x) - outside stack bounds\n", frame, ebp_addr);
            break;
        }

        uint32_t eip = ebp[1];  /* Return address */
        uint32_t prev_ebp = ebp[0];  /* Previous EBP */

        printk("Frame %d:\n", frame);
        printk("  EBP: 0x%x\n", ebp_addr);
        printk("  Return Address: 0x%x\n", eip);
        printk("  Previous EBP: 0x%x\n", prev_ebp);

        /* Move to previous frame */
        if (prev_ebp == 0 || prev_ebp == ebp_addr) {
            break;  /* End of stack or circular reference */
        }

        ebp = (uint32_t *)prev_ebp;
        frame++;
    }

    if (frame == 0) {
        printk("No stack frames to display\n");
    }
    printk("\n");
}
