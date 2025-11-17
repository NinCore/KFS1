/* pic.h - Programmable Interrupt Controller (8259 PIC) */

#ifndef PIC_H
#define PIC_H

#include "types.h"

/* PIC ports */
#define PIC1_COMMAND 0x20
#define PIC1_DATA    0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA    0xA1

/* PIC commands */
#define PIC_EOI      0x20  /* End of Interrupt */

/* ICW1 */
#define ICW1_ICW4      0x01  /* ICW4 needed */
#define ICW1_SINGLE    0x02  /* Single (cascade) mode */
#define ICW1_INTERVAL4 0x04  /* Call address interval 4 (8) */
#define ICW1_LEVEL     0x08  /* Level triggered (edge) mode */
#define ICW1_INIT      0x10  /* Initialization */

/* ICW4 */
#define ICW4_8086       0x01  /* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO       0x02  /* Auto (normal) EOI */
#define ICW4_BUF_SLAVE  0x08  /* Buffered mode/slave */
#define ICW4_BUF_MASTER 0x0C  /* Buffered mode/master */
#define ICW4_SFNM       0x10  /* Special fully nested */

/* Default IRQ mappings (before remapping) */
#define PIC1_OFFSET_OLD 0x08
#define PIC2_OFFSET_OLD 0x70

/* New IRQ mappings (after remapping to avoid conflicts with CPU exceptions) */
#define PIC1_OFFSET 0x20  /* Master PIC at 0x20-0x27 */
#define PIC2_OFFSET 0x28  /* Slave PIC at 0x28-0x2F */

/* Initialize and remap the PIC */
void pic_init(void);

/* Send End Of Interrupt to PIC */
void pic_send_eoi(uint8_t irq);

/* Mask (disable) an IRQ */
void pic_mask_irq(uint8_t irq);

/* Unmask (enable) an IRQ */
void pic_unmask_irq(uint8_t irq);

/* Disable all IRQs */
void pic_disable_all(void);

/* Get IRQ mask */
uint16_t pic_get_mask(void);

/* Set IRQ mask */
void pic_set_mask(uint16_t mask);

#endif /* PIC_H */
