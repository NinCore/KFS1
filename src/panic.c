/* panic.c - Kernel panic handling implementation */

#include "../include/panic.h"
#include "../include/printf.h"
#include "../include/vga.h"
#include "../include/string.h"

/* Clean all general purpose registers before halt */
void registers_clean(void) {
    __asm__ volatile(
        "xor %%eax, %%eax\n"
        "xor %%ebx, %%ebx\n"
        "xor %%ecx, %%ecx\n"
        "xor %%edx, %%edx\n"
        "xor %%esi, %%esi\n"
        "xor %%edi, %%edi\n"
        ::: "eax", "ebx", "ecx", "edx", "esi", "edi"
    );
}

/* Save current register state */
void registers_save(struct register_state *state) {
    if (!state) return;

    uint32_t temp_eax, temp_ebx, temp_ecx, temp_edx;
    uint32_t temp_esi, temp_edi, temp_ebp, temp_esp;

    __asm__ volatile("mov %%eax, %0" : "=r"(temp_eax));
    __asm__ volatile("mov %%ebx, %0" : "=r"(temp_ebx));
    __asm__ volatile("mov %%ecx, %0" : "=r"(temp_ecx));
    __asm__ volatile("mov %%edx, %0" : "=r"(temp_edx));
    __asm__ volatile("mov %%esi, %0" : "=r"(temp_esi));
    __asm__ volatile("mov %%edi, %0" : "=r"(temp_edi));
    __asm__ volatile("mov %%ebp, %0" : "=r"(temp_ebp));
    __asm__ volatile("mov %%esp, %0" : "=r"(temp_esp));

    state->eax = temp_eax;
    state->ebx = temp_ebx;
    state->ecx = temp_ecx;
    state->edx = temp_edx;
    state->esi = temp_esi;
    state->edi = temp_edi;
    state->ebp = temp_ebp;
    state->esp = temp_esp;

    /* Save segment registers */
    uint16_t temp_cs, temp_ds, temp_es, temp_fs, temp_gs, temp_ss;
    __asm__ volatile("mov %%cs, %0" : "=r"(temp_cs));
    __asm__ volatile("mov %%ds, %0" : "=r"(temp_ds));
    __asm__ volatile("mov %%es, %0" : "=r"(temp_es));
    __asm__ volatile("mov %%fs, %0" : "=r"(temp_fs));
    __asm__ volatile("mov %%gs, %0" : "=r"(temp_gs));
    __asm__ volatile("mov %%ss, %0" : "=r"(temp_ss));

    state->cs = temp_cs;
    state->ds = temp_ds;
    state->es = temp_es;
    state->fs = temp_fs;
    state->gs = temp_gs;
    state->ss = temp_ss;

    /* Save EIP (approximate - this is the return address) */
    uint32_t eip;
    __asm__ volatile(
        "call 1f\n"
        "1: pop %0\n"
        : "=r"(eip)
    );
    state->eip = eip;

    /* Save EFLAGS */
    uint32_t eflags;
    __asm__ volatile("pushfl; pop %0" : "=r"(eflags));
    state->eflags = eflags;
}

/* Restore register state */
void registers_restore(struct register_state *state) {
    if (!state) return;

    /* Restore segment registers */
    __asm__ volatile("mov %0, %%ds" :: "r"((uint32_t)state->ds));
    __asm__ volatile("mov %0, %%es" :: "r"((uint32_t)state->es));
    __asm__ volatile("mov %0, %%fs" :: "r"((uint32_t)state->fs));
    __asm__ volatile("mov %0, %%gs" :: "r"((uint32_t)state->gs));

    /* Restore general purpose registers */
    __asm__ volatile("mov %0, %%eax" :: "r"(state->eax));
    __asm__ volatile("mov %0, %%ebx" :: "r"(state->ebx));
    __asm__ volatile("mov %0, %%ecx" :: "r"(state->ecx));
    __asm__ volatile("mov %0, %%edx" :: "r"(state->edx));
    __asm__ volatile("mov %0, %%esi" :: "r"(state->esi));
    __asm__ volatile("mov %0, %%edi" :: "r"(state->edi));
    __asm__ volatile("mov %0, %%ebp" :: "r"(state->ebp));
}

/* Print register state */
void registers_print(struct register_state *state) {
    if (!state) return;

    printk("Register State:\n");
    printk("  EAX=0x%x  EBX=0x%x  ECX=0x%x  EDX=0x%x\n",
           state->eax, state->ebx, state->ecx, state->edx);
    printk("  ESI=0x%x  EDI=0x%x  EBP=0x%x  ESP=0x%x\n",
           state->esi, state->edi, state->ebp, state->esp);
    printk("  EIP=0x%x  EFLAGS=0x%x\n", state->eip, state->eflags);
    printk("  CS=0x%x  DS=0x%x  ES=0x%x  FS=0x%x  GS=0x%x  SS=0x%x\n",
           state->cs, state->ds, state->es, state->fs, state->gs, state->ss);
}

/* Save current stack snapshot */
void stack_save_snapshot(struct stack_snapshot *snapshot) {
    if (!snapshot) return;

    uint32_t esp, ebp;
    __asm__ volatile("mov %%esp, %0" : "=r"(esp));
    __asm__ volatile("mov %%ebp, %0" : "=r"(ebp));

    snapshot->esp = esp;
    snapshot->ebp = ebp;

    /* Copy stack data */
    uint32_t *stack_ptr = (uint32_t *)esp;
    snapshot->size = STACK_SAVE_SIZE;

    for (uint32_t i = 0; i < STACK_SAVE_SIZE && (uint32_t)&stack_ptr[i] < ebp + 0x1000; i++) {
        snapshot->data[i] = stack_ptr[i];
    }
}

/* Print stack snapshot */
void stack_print_snapshot(struct stack_snapshot *snapshot) {
    if (!snapshot) return;

    printk("\nStack Snapshot:\n");
    printk("  ESP: 0x%x\n", snapshot->esp);
    printk("  EBP: 0x%x\n", snapshot->ebp);
    printk("  Size: %d dwords\n\n", snapshot->size);

    printk("  Stack Contents (first 32 dwords):\n");
    for (uint32_t i = 0; i < 32 && i < snapshot->size; i++) {
        if (i % 4 == 0) {
            printk("\n  0x%x: ", snapshot->esp + (i * 4));
        }
        printk("0x%x ", snapshot->data[i]);
    }
    printk("\n");
}

/* Kernel panic - fatal error */
void kernel_panic(const char *message) {
    /* Save register state before panic */
    struct register_state regs;
    registers_save(&regs);

    /* Call panic with registers */
    kernel_panic_with_registers(message, &regs);
}

/* Kernel panic with register dump */
void kernel_panic_with_registers(const char *message, struct register_state *regs) {
    /* Disable interrupts */
    __asm__ volatile("cli");

    /* Save stack snapshot */
    struct stack_snapshot stack;
    stack_save_snapshot(&stack);

    /* Clear screen and display panic message */
    vga_clear();
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_RED);
    printk("\n\n");
    printk("  *** KERNEL PANIC ***  \n");
    printk("\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    printk("A fatal error has occurred and the kernel must stop.\n\n");
    printk("Error: %s\n\n", message);

    /* Print register state */
    if (regs) {
        registers_print(regs);
    }

    /* Print stack snapshot */
    stack_print_snapshot(&stack);

    printk("\nSystem halted. Cleaning registers...\n");

    /* Clean registers before halt */
    registers_clean();

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
