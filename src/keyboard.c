/* keyboard.c - PS/2 Keyboard driver implementation */

#include "../include/keyboard.h"
#include "../include/types.h"

/* Keyboard ports */
#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64

/* Port I/O functions */
static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

/* US QWERTY keyboard scancode map (lowercase) */
static const char scancode_to_ascii[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0,    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0,    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*',  0,  ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    '-',  0, 0, 0, '+', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/* US QWERTY keyboard scancode map (uppercase) */
static const char scancode_to_ascii_shift[128] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0,    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0,    '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*',  0,  ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    '-',  0, 0, 0, '+', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/* Keyboard state */
static bool shift_pressed = false;
static bool ctrl_pressed = false;
static bool alt_pressed = false;

void keyboard_init(void) {
    /* Keyboard is already initialized by BIOS, nothing to do */
    shift_pressed = false;
    ctrl_pressed = false;
    alt_pressed = false;
}

bool keyboard_haskey(void) {
    uint8_t status = inb(KEYBOARD_STATUS_PORT);
    return (status & 0x01) != 0;
}

int keyboard_getchar(void) {
    if (!keyboard_haskey()) {
        return 0;
    }

    uint8_t scancode = inb(KEYBOARD_DATA_PORT);

    /* Handle key release (bit 7 set) */
    if (scancode & 0x80) {
        scancode &= 0x7F;
        /* Check for shift/ctrl/alt release */
        if (scancode == 0x2A || scancode == 0x36) {
            shift_pressed = false;
        } else if (scancode == 0x1D) {
            ctrl_pressed = false;
        } else if (scancode == 0x38) {
            alt_pressed = false;
        }
        return 0;
    }

    /* Check for shift/ctrl/alt press */
    if (scancode == 0x2A || scancode == 0x36) {
        shift_pressed = true;
        return 0;
    } else if (scancode == 0x1D) {
        ctrl_pressed = true;
        return 0;
    } else if (scancode == 0x38) {
        alt_pressed = true;
        return 0;
    }

    /* Handle F-keys with Alt (for screen switching) */
    if (alt_pressed) {
        if (scancode >= 0x3B && scancode <= 0x44) {
            /* F1-F10 */
            return KEY_F1 + (scancode - 0x3B);
        }
    }

    /* Convert scancode to ASCII */
    char ascii;
    if (shift_pressed) {
        ascii = scancode_to_ascii_shift[scancode];
    } else {
        ascii = scancode_to_ascii[scancode];
    }

    return ascii;
}
