/* timer.c - Programmable Interval Timer implementation */

#include "../include/timer.h"
#include "../include/idt.h"
#include "../include/pic.h"
#include "../include/printf.h"
#include "../include/process.h"
#include "../include/io.h"

/* PIT I/O ports */
#define PIT_CHANNEL0 0x40  /* Channel 0 data port (used for system timer) */
#define PIT_COMMAND  0x43  /* Mode/Command register */

/* PIT command bits */
#define PIT_CMD_BINARY     0x00  /* Use binary counter (not BCD) */
#define PIT_CMD_MODE3      0x06  /* Mode 3: Square Wave Generator */
#define PIT_CMD_RW_BOTH    0x30  /* Read/Write: LSB then MSB */
#define PIT_CMD_CHANNEL0   0x00  /* Select channel 0 */

/* Timer ticks counter */
volatile uint32_t timer_ticks = 0;

/* Scheduling frequency (call scheduler every N ticks) */
#define SCHEDULE_FREQUENCY 10  /* Schedule every 10 ticks (100ms at 100Hz) */

/* Timer interrupt handler */
static void timer_irq_handler(struct interrupt_frame *frame) {
    (void)frame;  /* Unused */

    /* Increment tick counter */
    timer_ticks++;

    /* Call scheduler periodically for preemptive multitasking */
    if (timer_ticks % SCHEDULE_FREQUENCY == 0) {
        process_schedule();
    }

    /* Send EOI to PIC */
    pic_send_eoi(0);  /* IRQ0 */
}

/* Initialize the PIT timer */
void timer_init(uint32_t frequency) {
    /* Calculate divisor for desired frequency */
    uint32_t divisor = PIT_FREQUENCY / frequency;

    /* Ensure divisor is within valid range */
    if (divisor > 0xFFFF) {
        divisor = 0xFFFF;
    }
    if (divisor < 1) {
        divisor = 1;
    }

    /* Register IRQ0 handler */
    idt_register_handler(IRQ0, timer_irq_handler);

    /* Send command byte to PIT */
    outb(PIT_COMMAND, PIT_CMD_CHANNEL0 | PIT_CMD_RW_BOTH | PIT_CMD_MODE3 | PIT_CMD_BINARY);

    /* Send divisor (low byte then high byte) */
    outb(PIT_CHANNEL0, divisor & 0xFF);
    outb(PIT_CHANNEL0, (divisor >> 8) & 0xFF);

    /* Unmask IRQ0 (enable timer interrupts) */
    pic_unmask_irq(0);

    kernel_info("Timer initialized at %d Hz (divisor %d)", frequency, divisor);
}

/* Get current timer ticks */
uint32_t timer_get_ticks(void) {
    return timer_ticks;
}

/* Wait for specified number of ticks */
void timer_wait(uint32_t ticks) {
    uint32_t end_ticks = timer_ticks + ticks;
    while (timer_ticks < end_ticks) {
        /* Busy wait */
        __asm__ volatile("hlt");  /* Halt until next interrupt */
    }
}
