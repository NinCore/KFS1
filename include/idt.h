/* idt.h - Interrupt Descriptor Table definitions and functions */

#ifndef IDT_H
#define IDT_H

#include "types.h"

/* IDT Entry Structure (8 bytes) */
struct idt_entry {
    uint16_t offset_low;     /* Lower 16 bits of handler address */
    uint16_t selector;       /* Kernel segment selector */
    uint8_t  zero;           /* Always 0 */
    uint8_t  type_attr;      /* Type and attributes */
    uint16_t offset_high;    /* Upper 16 bits of handler address */
} __attribute__((packed));

/* IDT Pointer Structure */
struct idt_ptr {
    uint16_t limit;          /* Size of IDT - 1 */
    uint32_t base;           /* Address of the first IDT entry */
} __attribute__((packed));

/* Interrupt frame pushed by CPU on interrupt */
struct interrupt_frame {
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t int_no;         /* Interrupt number */
    uint32_t err_code;       /* Error code (if applicable) */
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
    uint32_t user_esp;
    uint32_t user_ss;
} __attribute__((packed));

/* Number of IDT entries (0-255) */
#define IDT_ENTRIES 256

/* IDT Type attributes */
#define IDT_PRESENT     0x80  /* Segment is present */
#define IDT_RING0       0x00  /* Ring 0 (kernel) */
#define IDT_RING3       0x60  /* Ring 3 (user) */
#define IDT_INTERRUPT   0x0E  /* 32-bit interrupt gate */
#define IDT_TRAP        0x0F  /* 32-bit trap gate */

/* Exception numbers (0x00-0x1F) */
#define EXC_DIVIDE_BY_ZERO          0x00
#define EXC_DEBUG                   0x01
#define EXC_NMI                     0x02
#define EXC_BREAKPOINT              0x03
#define EXC_OVERFLOW                0x04
#define EXC_BOUND_RANGE             0x05
#define EXC_INVALID_OPCODE          0x06
#define EXC_DEVICE_NOT_AVAILABLE    0x07
#define EXC_DOUBLE_FAULT            0x08
#define EXC_COPROCESSOR_OVERRUN     0x09
#define EXC_INVALID_TSS             0x0A
#define EXC_SEGMENT_NOT_PRESENT     0x0B
#define EXC_STACK_FAULT             0x0C
#define EXC_GENERAL_PROTECTION      0x0D
#define EXC_PAGE_FAULT              0x0E
#define EXC_RESERVED                0x0F
#define EXC_FPU_ERROR               0x10
#define EXC_ALIGNMENT_CHECK         0x11
#define EXC_MACHINE_CHECK           0x12
#define EXC_SIMD_FP_EXCEPTION       0x13

/* Hardware IRQ numbers (remapped to 0x20-0x2F) */
#define IRQ0  0x20  /* Timer */
#define IRQ1  0x21  /* Keyboard */
#define IRQ2  0x22  /* Cascade */
#define IRQ3  0x23  /* COM2 */
#define IRQ4  0x24  /* COM1 */
#define IRQ5  0x25  /* LPT2 */
#define IRQ6  0x26  /* Floppy */
#define IRQ7  0x27  /* LPT1 */
#define IRQ8  0x28  /* RTC */
#define IRQ9  0x29  /* Free */
#define IRQ10 0x2A  /* Free */
#define IRQ11 0x2B  /* Free */
#define IRQ12 0x2C  /* PS/2 Mouse */
#define IRQ13 0x2D  /* FPU */
#define IRQ14 0x2E  /* Primary ATA */
#define IRQ15 0x2F  /* Secondary ATA */

/* Software interrupt for syscalls */
#define INT_SYSCALL 0x80

/* Interrupt handler function type */
typedef void (*interrupt_handler_t)(struct interrupt_frame *frame);

/* Signal handler function type */
typedef void (*signal_handler_t)(int signal);

/* Initialize the IDT */
void idt_init(void);

/* Set an IDT entry */
void idt_set_gate(uint8_t num, uint32_t handler, uint16_t selector, uint8_t flags);

/* Register a C interrupt handler */
void idt_register_handler(uint8_t num, interrupt_handler_t handler);

/* Unregister a C interrupt handler */
void idt_unregister_handler(uint8_t num);

/* Enable/disable interrupts */
void interrupts_enable(void);
void interrupts_disable(void);

/* Check if interrupts are enabled */
bool interrupts_enabled(void);

/* External assembly interrupt stubs */
extern void isr0(void);   /* Division by Zero */
extern void isr1(void);   /* Debug */
extern void isr2(void);   /* NMI */
extern void isr3(void);   /* Breakpoint */
extern void isr4(void);   /* Overflow */
extern void isr5(void);   /* Bound Range */
extern void isr6(void);   /* Invalid Opcode */
extern void isr7(void);   /* Device Not Available */
extern void isr8(void);   /* Double Fault */
extern void isr9(void);   /* Coprocessor Overrun */
extern void isr10(void);  /* Invalid TSS */
extern void isr11(void);  /* Segment Not Present */
extern void isr12(void);  /* Stack Fault */
extern void isr13(void);  /* General Protection */
extern void isr14(void);  /* Page Fault */
extern void isr15(void);  /* Reserved */
extern void isr16(void);  /* FPU Error */
extern void isr17(void);  /* Alignment Check */
extern void isr18(void);  /* Machine Check */
extern void isr19(void);  /* SIMD FP Exception */

/* IRQ handlers */
extern void irq0(void);   /* Timer */
extern void irq1(void);   /* Keyboard */
extern void irq2(void);   /* Cascade */
extern void irq3(void);   /* COM2 */
extern void irq4(void);   /* COM1 */
extern void irq5(void);   /* LPT2 */
extern void irq6(void);   /* Floppy */
extern void irq7(void);   /* LPT1 */
extern void irq8(void);   /* RTC */
extern void irq9(void);   /* Free */
extern void irq10(void);  /* Free */
extern void irq11(void);  /* Free */
extern void irq12(void);  /* PS/2 Mouse */
extern void irq13(void);  /* FPU */
extern void irq14(void);  /* Primary ATA */
extern void irq15(void);  /* Secondary ATA */

/* Syscall handler */
extern void isr128(void); /* Syscall (INT 0x80) */

/* Print IDT information for debugging */
void idt_print_info(void);

#endif /* IDT_H */
