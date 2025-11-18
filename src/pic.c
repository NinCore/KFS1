/* pic.c - Programmable Interrupt Controller implementation */

#include "../include/pic.h"
#include "../include/io.h"

/* Wait for PIC to finish processing */
static void pic_wait(void) {
    /* Short delay using port 0x80 (unused port) */
    outb(0x80, 0);
}

/* Initialize and remap the PIC */
void pic_init(void) {
    /* Save masks */
    uint8_t mask1 = inb(PIC1_DATA);
    uint8_t mask2 = inb(PIC2_DATA);

    /* Start initialization sequence (ICW1) */
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    pic_wait();
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    pic_wait();

    /* Set vector offsets (ICW2) */
    outb(PIC1_DATA, PIC1_OFFSET);  /* Master PIC offset to 0x20 */
    pic_wait();
    outb(PIC2_DATA, PIC2_OFFSET);  /* Slave PIC offset to 0x28 */
    pic_wait();

    /* Tell Master PIC there's a slave at IRQ2 (ICW3) */
    outb(PIC1_DATA, 0x04);  /* Slave on IRQ2 */
    pic_wait();
    /* Tell Slave PIC its cascade identity (ICW3) */
    outb(PIC2_DATA, 0x02);  /* Cascade identity */
    pic_wait();

    /* Set 8086 mode (ICW4) */
    outb(PIC1_DATA, ICW4_8086);
    pic_wait();
    outb(PIC2_DATA, ICW4_8086);
    pic_wait();

    /* Restore saved masks */
    outb(PIC1_DATA, mask1);
    outb(PIC2_DATA, mask2);
}

/* Send End Of Interrupt to appropriate PIC */
void pic_send_eoi(uint8_t irq) {
    /* If IRQ came from slave PIC, send EOI to both */
    if (irq >= 8) {
        outb(PIC2_COMMAND, PIC_EOI);
    }
    /* Always send EOI to master */
    outb(PIC1_COMMAND, PIC_EOI);
}

/* Mask (disable) an IRQ */
void pic_mask_irq(uint8_t irq) {
    uint16_t port;
    uint8_t value;

    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }

    value = inb(port) | (1 << irq);
    outb(port, value);
}

/* Unmask (enable) an IRQ */
void pic_unmask_irq(uint8_t irq) {
    uint16_t port;
    uint8_t value;

    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }

    value = inb(port) & ~(1 << irq);
    outb(port, value);
}

/* Disable all IRQs */
void pic_disable_all(void) {
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
}

/* Get current IRQ mask */
uint16_t pic_get_mask(void) {
    return (uint16_t)inb(PIC1_DATA) | ((uint16_t)inb(PIC2_DATA) << 8);
}

/* Set IRQ mask */
void pic_set_mask(uint16_t mask) {
    outb(PIC1_DATA, mask & 0xFF);
    outb(PIC2_DATA, (mask >> 8) & 0xFF);
}
