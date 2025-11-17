/* idt.c - Interrupt Descriptor Table implementation */

#include "../include/idt.h"
#include "../include/pic.h"
#include "../include/printf.h"
#include "../include/vga.h"
#include "../include/string.h"
#include "../include/panic.h"

/* IDT entries array */
static struct idt_entry idt_entries[IDT_ENTRIES];
static struct idt_ptr idt_pointer;

/* C-level interrupt handlers */
static interrupt_handler_t interrupt_handlers[IDT_ENTRIES];

/* Exception messages */
static const char *exception_messages[] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack Fault",
    "General Protection Fault",
    "Page Fault",
    "Reserved",
    "x87 FPU Error",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception"
};

/* Set an IDT entry */
void idt_set_gate(uint8_t num, uint32_t handler, uint16_t selector, uint8_t flags) {
    idt_entries[num].offset_low = handler & 0xFFFF;
    idt_entries[num].offset_high = (handler >> 16) & 0xFFFF;
    idt_entries[num].selector = selector;
    idt_entries[num].zero = 0;
    idt_entries[num].type_attr = flags | IDT_PRESENT;
}

/* Register a C interrupt handler */
void idt_register_handler(uint8_t num, interrupt_handler_t handler) {
    interrupt_handlers[num] = handler;
}

/* Unregister a C interrupt handler */
void idt_unregister_handler(uint8_t num) {
    interrupt_handlers[num] = NULL;
}

/* Default exception handler */
static void default_exception_handler(struct interrupt_frame *frame) {
    /* Disable interrupts */
    interrupts_disable();

    /* Clear screen and display exception information */
    vga_clear();
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_RED);
    printk("\n\n  *** CPU EXCEPTION ***  \n\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    /* Display exception details */
    if (frame->int_no < 20) {
        printk("Exception: %s\n", exception_messages[frame->int_no]);
    } else {
        printk("Exception: Unknown (0x%x)\n", frame->int_no);
    }

    printk("Error Code: 0x%x\n\n", frame->err_code);

    /* Display register dump */
    printk("Register Dump:\n");
    printk("  EAX=0x%x  EBX=0x%x  ECX=0x%x  EDX=0x%x\n",
           frame->eax, frame->ebx, frame->ecx, frame->edx);
    printk("  ESI=0x%x  EDI=0x%x  EBP=0x%x  ESP=0x%x\n",
           frame->esi, frame->edi, frame->ebp, frame->esp);
    printk("  EIP=0x%x  EFLAGS=0x%x\n", frame->eip, frame->eflags);
    printk("  CS=0x%x  SS=0x%x\n\n", frame->cs, frame->user_ss);

    /* Page fault specific information */
    if (frame->int_no == EXC_PAGE_FAULT) {
        uint32_t faulting_address;
        __asm__ volatile("mov %%cr2, %0" : "=r"(faulting_address));
        printk("Page Fault Details:\n");
        printk("  Faulting Address: 0x%x\n", faulting_address);
        printk("  Error Code:\n");
        printk("    Present: %s\n", (frame->err_code & 0x1) ? "Yes" : "No");
        printk("    Write: %s\n", (frame->err_code & 0x2) ? "Yes" : "No");
        printk("    User: %s\n", (frame->err_code & 0x4) ? "Yes" : "No");
        printk("    Reserved: %s\n", (frame->err_code & 0x8) ? "Yes" : "No");
        printk("    Instruction Fetch: %s\n\n", (frame->err_code & 0x10) ? "Yes" : "No");
    }

    printk("System halted.\n");

    /* Halt the system */
    while (1) {
        __asm__ volatile("hlt");
    }
}

/* Default IRQ handler */
static void default_irq_handler(struct interrupt_frame *frame) {
    /* Send EOI to PIC */
    if (frame->int_no >= 32 && frame->int_no <= 47) {
        pic_send_eoi(frame->int_no - 32);
    }
}

/* Common interrupt handler called from assembly */
void interrupt_handler_common(struct interrupt_frame *frame) {
    /* Check if we have a registered handler */
    if (interrupt_handlers[frame->int_no] != NULL) {
        /* Call the registered handler */
        interrupt_handlers[frame->int_no](frame);
    } else {
        /* Use default handlers */
        if (frame->int_no < 32) {
            /* CPU exception */
            default_exception_handler(frame);
        } else if (frame->int_no >= 32 && frame->int_no <= 47) {
            /* Hardware IRQ */
            default_irq_handler(frame);
        }
    }
}

/* Initialize the IDT */
void idt_init(void) {
    /* Set up IDT pointer */
    idt_pointer.limit = (sizeof(struct idt_entry) * IDT_ENTRIES) - 1;
    idt_pointer.base = (uint32_t)&idt_entries;

    /* Clear IDT entries and handlers */
    for (int i = 0; i < IDT_ENTRIES; i++) {
        idt_entries[i].offset_low = 0;
        idt_entries[i].offset_high = 0;
        idt_entries[i].selector = 0;
        idt_entries[i].zero = 0;
        idt_entries[i].type_attr = 0;
        interrupt_handlers[i] = NULL;
    }

    /* Remap PIC */
    pic_init();

    /* Install exception handlers (ISR 0-19) */
    idt_set_gate(0, (uint32_t)isr0, 0x08, IDT_INTERRUPT | IDT_RING0);
    idt_set_gate(1, (uint32_t)isr1, 0x08, IDT_INTERRUPT | IDT_RING0);
    idt_set_gate(2, (uint32_t)isr2, 0x08, IDT_INTERRUPT | IDT_RING0);
    idt_set_gate(3, (uint32_t)isr3, 0x08, IDT_INTERRUPT | IDT_RING0);
    idt_set_gate(4, (uint32_t)isr4, 0x08, IDT_INTERRUPT | IDT_RING0);
    idt_set_gate(5, (uint32_t)isr5, 0x08, IDT_INTERRUPT | IDT_RING0);
    idt_set_gate(6, (uint32_t)isr6, 0x08, IDT_INTERRUPT | IDT_RING0);
    idt_set_gate(7, (uint32_t)isr7, 0x08, IDT_INTERRUPT | IDT_RING0);
    idt_set_gate(8, (uint32_t)isr8, 0x08, IDT_INTERRUPT | IDT_RING0);
    idt_set_gate(9, (uint32_t)isr9, 0x08, IDT_INTERRUPT | IDT_RING0);
    idt_set_gate(10, (uint32_t)isr10, 0x08, IDT_INTERRUPT | IDT_RING0);
    idt_set_gate(11, (uint32_t)isr11, 0x08, IDT_INTERRUPT | IDT_RING0);
    idt_set_gate(12, (uint32_t)isr12, 0x08, IDT_INTERRUPT | IDT_RING0);
    idt_set_gate(13, (uint32_t)isr13, 0x08, IDT_INTERRUPT | IDT_RING0);
    idt_set_gate(14, (uint32_t)isr14, 0x08, IDT_INTERRUPT | IDT_RING0);
    idt_set_gate(15, (uint32_t)isr15, 0x08, IDT_INTERRUPT | IDT_RING0);
    idt_set_gate(16, (uint32_t)isr16, 0x08, IDT_INTERRUPT | IDT_RING0);
    idt_set_gate(17, (uint32_t)isr17, 0x08, IDT_INTERRUPT | IDT_RING0);
    idt_set_gate(18, (uint32_t)isr18, 0x08, IDT_INTERRUPT | IDT_RING0);
    idt_set_gate(19, (uint32_t)isr19, 0x08, IDT_INTERRUPT | IDT_RING0);

    /* Install IRQ handlers (IRQ 0-15, mapped to interrupts 32-47) */
    idt_set_gate(32, (uint32_t)irq0, 0x08, IDT_INTERRUPT | IDT_RING0);
    idt_set_gate(33, (uint32_t)irq1, 0x08, IDT_INTERRUPT | IDT_RING0);
    idt_set_gate(34, (uint32_t)irq2, 0x08, IDT_INTERRUPT | IDT_RING0);
    idt_set_gate(35, (uint32_t)irq3, 0x08, IDT_INTERRUPT | IDT_RING0);
    idt_set_gate(36, (uint32_t)irq4, 0x08, IDT_INTERRUPT | IDT_RING0);
    idt_set_gate(37, (uint32_t)irq5, 0x08, IDT_INTERRUPT | IDT_RING0);
    idt_set_gate(38, (uint32_t)irq6, 0x08, IDT_INTERRUPT | IDT_RING0);
    idt_set_gate(39, (uint32_t)irq7, 0x08, IDT_INTERRUPT | IDT_RING0);
    idt_set_gate(40, (uint32_t)irq8, 0x08, IDT_INTERRUPT | IDT_RING0);
    idt_set_gate(41, (uint32_t)irq9, 0x08, IDT_INTERRUPT | IDT_RING0);
    idt_set_gate(42, (uint32_t)irq10, 0x08, IDT_INTERRUPT | IDT_RING0);
    idt_set_gate(43, (uint32_t)irq11, 0x08, IDT_INTERRUPT | IDT_RING0);
    idt_set_gate(44, (uint32_t)irq12, 0x08, IDT_INTERRUPT | IDT_RING0);
    idt_set_gate(45, (uint32_t)irq13, 0x08, IDT_INTERRUPT | IDT_RING0);
    idt_set_gate(46, (uint32_t)irq14, 0x08, IDT_INTERRUPT | IDT_RING0);
    idt_set_gate(47, (uint32_t)irq15, 0x08, IDT_INTERRUPT | IDT_RING0);

    /* Install syscall handler (INT 0x80) - Ring 3 accessible */
    idt_set_gate(128, (uint32_t)isr128, 0x08, IDT_INTERRUPT | IDT_RING3);

    /* Load IDT */
    __asm__ volatile("lidt %0" : : "m"(idt_pointer));

    /* Mask all IRQs initially */
    pic_disable_all();
}

/* Enable interrupts */
void interrupts_enable(void) {
    __asm__ volatile("sti");
}

/* Disable interrupts */
void interrupts_disable(void) {
    __asm__ volatile("cli");
}

/* Check if interrupts are enabled */
bool interrupts_enabled(void) {
    uint32_t flags;
    __asm__ volatile("pushfl; pop %0" : "=r"(flags));
    return (flags & 0x200) != 0;  /* Check IF flag */
}

/* Print IDT information for debugging */
void idt_print_info(void) {
    printk("\n=== Interrupt Descriptor Table ===\n");
    printk("IDT Base Address: 0x%x\n", idt_pointer.base);
    printk("IDT Limit: %d bytes\n", idt_pointer.limit + 1);
    printk("Number of Entries: %d\n\n", IDT_ENTRIES);

    printk("Exception Handlers (0-19):\n");
    for (int i = 0; i < 20; i++) {
        uint32_t offset = idt_entries[i].offset_low | (idt_entries[i].offset_high << 16);
        if (offset != 0) {
            printk("  ISR %d: 0x%x - %s\n", i, offset, exception_messages[i]);
        }
    }

    printk("\nIRQ Handlers (32-47):\n");
    for (int i = 32; i <= 47; i++) {
        uint32_t offset = idt_entries[i].offset_low | (idt_entries[i].offset_high << 16);
        if (offset != 0) {
            printk("  IRQ %d: 0x%x\n", i - 32, offset);
        }
    }

    printk("\nSyscall Handler:\n");
    uint32_t syscall_offset = idt_entries[128].offset_low | (idt_entries[128].offset_high << 16);
    if (syscall_offset != 0) {
        printk("  INT 0x80: 0x%x\n", syscall_offset);
    }

    printk("\nInterrupts: %s\n", interrupts_enabled() ? "Enabled" : "Disabled");
}
