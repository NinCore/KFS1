/* mouse.h - PS/2 Mouse support for scrolling */

#ifndef MOUSE_H
#define MOUSE_H

#include "types.h"

/* Mouse packet structure */
struct mouse_packet {
    uint8_t flags;
    int8_t x_movement;
    int8_t y_movement;
    int8_t z_movement;  /* Scroll wheel */
};

/* Mouse flags */
#define MOUSE_LEFT_BUTTON    0x01
#define MOUSE_RIGHT_BUTTON   0x02
#define MOUSE_MIDDLE_BUTTON  0x04
#define MOUSE_ALWAYS_1       0x08
#define MOUSE_X_SIGN         0x10
#define MOUSE_Y_SIGN         0x20
#define MOUSE_X_OVERFLOW     0x40
#define MOUSE_Y_OVERFLOW     0x80

/* Initialize mouse */
void mouse_init(void);

/* Enable mouse interrupts */
void mouse_enable_interrupts(void);

/* Disable mouse interrupts */
void mouse_disable_interrupts(void);

/* Get scroll delta (positive = up, negative = down) */
int mouse_get_scroll_delta(void);

#endif /* MOUSE_H */
