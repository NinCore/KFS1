# interrupt.s - Interrupt Service Routine stubs
# This file defines the low-level interrupt handlers

.section .text

# Common ISR stub that saves processor state, calls C handler, and restores state
.extern interrupt_handler_common
.global isr_common_stub

isr_common_stub:
    # Save all general purpose registers
    pushal

    # Save segment registers
    push %ds
    push %es
    push %fs
    push %gs

    # Load kernel data segment
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    # Call C interrupt handler (interrupt_frame* is on stack)
    push %esp
    call interrupt_handler_common
    add $4, %esp

    # Restore segment registers
    pop %gs
    pop %fs
    pop %es
    pop %ds

    # Restore general purpose registers
    popal

    # Clean up error code and interrupt number
    add $8, %esp

    # Return from interrupt
    iret

# Macro to define ISR without error code
.macro ISR_NOERRCODE num
.global isr\num
isr\num:
    cli
    push $0         # Push dummy error code
    push $\num      # Push interrupt number
    jmp isr_common_stub
.endm

# Macro to define ISR with error code
.macro ISR_ERRCODE num
.global isr\num
isr\num:
    cli
    push $\num      # Push interrupt number (error code already pushed by CPU)
    jmp isr_common_stub
.endm

# Macro to define IRQ handler
.macro IRQ num irq_num
.global irq\num
irq\num:
    cli
    push $0         # Push dummy error code
    push $\irq_num  # Push IRQ number (offset by 32)
    jmp isr_common_stub
.endm

# CPU Exception ISRs (0-19)
ISR_NOERRCODE 0   # Division By Zero
ISR_NOERRCODE 1   # Debug
ISR_NOERRCODE 2   # Non Maskable Interrupt
ISR_NOERRCODE 3   # Breakpoint
ISR_NOERRCODE 4   # Overflow
ISR_NOERRCODE 5   # Bound Range Exceeded
ISR_NOERRCODE 6   # Invalid Opcode
ISR_NOERRCODE 7   # Device Not Available
ISR_ERRCODE   8   # Double Fault
ISR_NOERRCODE 9   # Coprocessor Segment Overrun
ISR_ERRCODE   10  # Invalid TSS
ISR_ERRCODE   11  # Segment Not Present
ISR_ERRCODE   12  # Stack Fault
ISR_ERRCODE   13  # General Protection Fault
ISR_ERRCODE   14  # Page Fault
ISR_NOERRCODE 15  # Reserved
ISR_NOERRCODE 16  # x87 FPU Error
ISR_ERRCODE   17  # Alignment Check
ISR_NOERRCODE 18  # Machine Check
ISR_NOERRCODE 19  # SIMD Floating-Point Exception

# IRQ handlers (32-47, mapped to 0x20-0x2F)
IRQ 0,  32   # IRQ0  - Timer
IRQ 1,  33   # IRQ1  - Keyboard
IRQ 2,  34   # IRQ2  - Cascade
IRQ 3,  35   # IRQ3  - COM2
IRQ 4,  36   # IRQ4  - COM1
IRQ 5,  37   # IRQ5  - LPT2
IRQ 6,  38   # IRQ6  - Floppy
IRQ 7,  39   # IRQ7  - LPT1
IRQ 8,  40   # IRQ8  - RTC
IRQ 9,  41   # IRQ9  - Free
IRQ 10, 42   # IRQ10 - Free
IRQ 11, 43   # IRQ11 - Free
IRQ 12, 44   # IRQ12 - PS/2 Mouse
IRQ 13, 45   # IRQ13 - FPU
IRQ 14, 46   # IRQ14 - Primary ATA
IRQ 15, 47   # IRQ15 - Secondary ATA

# Syscall handler (INT 0x80)
ISR_NOERRCODE 128
