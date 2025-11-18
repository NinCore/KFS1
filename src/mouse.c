/* mouse.c - PS/2 Mouse support implementation */

#include "../include/mouse.h"
#include "../include/io.h"
#include "../include/idt.h"
#include "../include/printf.h"
#include "../include/scrollback.h"

/* PS/2 Controller ports */
#define PS2_DATA_PORT    0x60
#define PS2_STATUS_PORT  0x64
#define PS2_COMMAND_PORT 0x64

/* Mouse packet state */
static uint8_t mouse_cycle = 0;
static int8_t mouse_byte[4];
static int scroll_delta = 0;

/* Wait for PS/2 controller to be ready for input */
static void mouse_wait_input(void) {
    uint32_t timeout = 100000;
    while (timeout--) {
        if ((inb(PS2_STATUS_PORT) & 2) == 0) {
            return;
        }
    }
}

/* Wait for PS/2 controller to have output */
static void mouse_wait_output(void) {
    uint32_t timeout = 100000;
    while (timeout--) {
        if ((inb(PS2_STATUS_PORT) & 1) == 1) {
            return;
        }
    }
}

/* Write to mouse */
static void mouse_write(uint8_t data) {
    mouse_wait_input();
    outb(PS2_COMMAND_PORT, 0xD4);
    mouse_wait_input();
    outb(PS2_DATA_PORT, data);
}

/* Read from mouse */
static uint8_t mouse_read(void) {
    mouse_wait_output();
    return inb(PS2_DATA_PORT);
}

/* Mouse interrupt handler (IRQ12) */
static void mouse_handler(struct interrupt_frame *frame) {
    (void)frame;

    uint8_t status = inb(PS2_STATUS_PORT);
    if (!(status & 0x20)) {
        return;  /* Not from mouse */
    }

    uint8_t data = inb(PS2_DATA_PORT);

    switch (mouse_cycle) {
        case 0:
            /* First byte - flags */
            mouse_byte[0] = data;
            /* Check if bit 3 is set (always 1 in first byte) */
            if (data & MOUSE_ALWAYS_1) {
                mouse_cycle++;
            }
            break;

        case 1:
            /* Second byte - X movement */
            mouse_byte[1] = data;
            mouse_cycle++;
            break;

        case 2:
            /* Third byte - Y movement */
            mouse_byte[2] = data;
            mouse_cycle++;
            break;

        case 3:
            /* Fourth byte - Z movement (scroll wheel) */
            mouse_byte[3] = data;

            /* Process scroll wheel */
            int8_t z = (int8_t)mouse_byte[3];
            if (z != 0) {
                /* Positive Z = scroll up, negative = scroll down */
                /* We invert it because scrollback expects positive = up */
                scroll_delta = -z;
                scrollback_scroll(scroll_delta);
                scroll_delta = 0;
            }

            /* Reset for next packet */
            mouse_cycle = 0;
            break;
    }
}

void mouse_init(void) {
    uint8_t status;

    /* Save keyboard state */
    mouse_wait_input();
    outb(PS2_COMMAND_PORT, 0x20);
    mouse_wait_output();
    uint8_t config = inb(PS2_DATA_PORT);

    /* Enable auxiliary device (mouse) */
    mouse_wait_input();
    outb(PS2_COMMAND_PORT, 0xA8);

    /* Enable interrupts for auxiliary device, preserve keyboard interrupt */
    mouse_wait_input();
    outb(PS2_COMMAND_PORT, 0x20);
    mouse_wait_output();
    status = inb(PS2_DATA_PORT);
    status |= 0x02;  /* Enable mouse interrupt */
    status |= 0x01;  /* Keep keyboard interrupt enabled */
    mouse_wait_input();
    outb(PS2_COMMAND_PORT, 0x60);
    mouse_wait_input();
    outb(PS2_DATA_PORT, status);

    /* Use default settings */
    mouse_write(0xF6);
    mouse_read();  /* Acknowledge */

    /* Enable scroll wheel (4-byte packets) */
    mouse_write(0xF3);  /* Set sample rate */
    mouse_read();
    mouse_write(200);   /* Sample rate 200 */
    mouse_read();
    mouse_write(0xF3);  /* Set sample rate */
    mouse_read();
    mouse_write(100);   /* Sample rate 100 */
    mouse_read();
    mouse_write(0xF3);  /* Set sample rate */
    mouse_read();
    mouse_write(80);    /* Sample rate 80 */
    mouse_read();

    /* Read device ID - should be 3 for scroll wheel support */
    mouse_write(0xF2);
    mouse_read();  /* Acknowledge */
    mouse_read();  /* Device ID */

    /* Enable mouse data packets */
    mouse_write(0xF4);
    mouse_read();  /* Acknowledge */

    /* Register mouse interrupt handler (IRQ12) */
    idt_register_handler(IRQ12, mouse_handler);

    mouse_cycle = 0;
    scroll_delta = 0;

    /* Restore configuration to ensure keyboard still works */
    (void)config;
}

void mouse_enable_interrupts(void) {
    /* Enable IRQ12 in PIC */
    uint8_t mask = inb(0xA1);
    mask &= ~(1 << 4);  /* Clear bit 4 (IRQ12) */
    outb(0xA1, mask);
}

void mouse_disable_interrupts(void) {
    /* Disable IRQ12 in PIC */
    uint8_t mask = inb(0xA1);
    mask |= (1 << 4);  /* Set bit 4 (IRQ12) */
    outb(0xA1, mask);
}

int mouse_get_scroll_delta(void) {
    int delta = scroll_delta;
    scroll_delta = 0;
    return delta;
}
