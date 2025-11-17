/* keyboard_irq.c - Interrupt-driven keyboard driver with multi-layout support */

#include "../include/keyboard.h"
#include "../include/idt.h"
#include "../include/pic.h"
#include "../include/io.h"
#include "../include/printf.h"
#include "../include/vga.h"

/* Keyboard ports */
#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64

/* US QWERTY keyboard scancode map */
static const char scancode_to_ascii_qwerty[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0,    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0,    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*',  0,  ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    '-',  0, 0, 0, '+', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static const char scancode_to_ascii_qwerty_shift[128] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0,    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0,    '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*',  0,  ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    '-',  0, 0, 0, '+', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/* French AZERTY keyboard scancode map */
static const char scancode_to_ascii_azerty[128] = {
    0,  27, '&', 'e', '"', '\'', '(', '-', 'e', '_', 'c', 'a', ')', '=', '\b',
    '\t', 'a', 'z', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '^', '$', '\n',
    0,    'q', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', 'm', 'u', '`',
    0,    '*', 'w', 'x', 'c', 'v', 'b', 'n', ',', ';', ':', '!', 0,
    '*',  0,  ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    '-',  0, 0, 0, '+', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static const char scancode_to_ascii_azerty_shift[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', 'o', '+', '\b',
    '\t', 'A', 'Z', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '"', 'L', '\n',
    0,    'Q', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', 'M', '%', 'P',
    0,    'u', 'W', 'X', 'C', 'V', 'B', 'N', '?', '.', '/', 'S', 0,
    '*',  0,  ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    '-',  0, 0, 0, '+', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/* German QWERTZ keyboard scancode map */
static const char scancode_to_ascii_qwertz[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', 's', '\'', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'z', 'u', 'i', 'o', 'p', 'u', '+', '\n',
    0,    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', 'o', 'a', '^',
    0,    '#', 'y', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '-', 0,
    '*',  0,  ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    '-',  0, 0, 0, '+', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static const char scancode_to_ascii_qwertz_shift[128] = {
    0,  27, '!', '"', 'P', '$', '%', '&', '/', '(', ')', '=', '?', '`', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Z', 'U', 'I', 'O', 'P', 'U', '*', '\n',
    0,    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', 'O', 'A', 'o',
    0,    '\'', 'Y', 'X', 'C', 'V', 'B', 'N', 'M', ';', ':', '_', 0,
    '*',  0,  ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    '-',  0, 0, 0, '+', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/* Keyboard state */
static bool shift_pressed = false;
static bool ctrl_pressed = false;
static bool alt_pressed = false;
static keyboard_layout_t current_layout = KEYBOARD_LAYOUT_QWERTY;

/* Circular keyboard buffer for interrupt-driven input */
static int keyboard_buffer[KEYBOARD_BUFFER_SIZE];
static int buffer_read_pos = 0;
static int buffer_write_pos = 0;

/* Buffer helper functions */
static inline bool buffer_is_empty(void) {
    return buffer_read_pos == buffer_write_pos;
}

static inline bool buffer_is_full(void) {
    return ((buffer_write_pos + 1) % KEYBOARD_BUFFER_SIZE) == buffer_read_pos;
}

static inline void buffer_put(int c) {
    if (!buffer_is_full()) {
        keyboard_buffer[buffer_write_pos] = c;
        buffer_write_pos = (buffer_write_pos + 1) % KEYBOARD_BUFFER_SIZE;
    }
}

static inline int buffer_get(void) {
    if (buffer_is_empty()) {
        return 0;
    }
    int c = keyboard_buffer[buffer_read_pos];
    buffer_read_pos = (buffer_read_pos + 1) % KEYBOARD_BUFFER_SIZE;
    return c;
}

/* Get scancode to ASCII map based on current layout */
static const char* get_scancode_map(bool shifted) {
    switch (current_layout) {
        case KEYBOARD_LAYOUT_AZERTY:
            return shifted ? scancode_to_ascii_azerty_shift : scancode_to_ascii_azerty;
        case KEYBOARD_LAYOUT_QWERTZ:
            return shifted ? scancode_to_ascii_qwertz_shift : scancode_to_ascii_qwertz;
        case KEYBOARD_LAYOUT_QWERTY:
        default:
            return shifted ? scancode_to_ascii_qwerty_shift : scancode_to_ascii_qwerty;
    }
}

/* Keyboard interrupt handler (called from IRQ1) */
void keyboard_interrupt_handler(void) {
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
        return;
    }

    /* Check for shift/ctrl/alt press */
    if (scancode == 0x2A || scancode == 0x36) {
        shift_pressed = true;
        return;
    } else if (scancode == 0x1D) {
        ctrl_pressed = true;
        return;
    } else if (scancode == 0x38) {
        alt_pressed = true;
        return;
    }

    /* Handle F-keys with Alt (for screen switching) */
    if (alt_pressed) {
        if (scancode >= 0x3B && scancode <= 0x44) {
            /* F1-F10 */
            buffer_put(KEY_F1 + (scancode - 0x3B));
            return;
        }
    }

    /* Convert scancode to ASCII using current layout */
    const char *map = get_scancode_map(shift_pressed);
    char ascii = map[scancode];

    if (ascii != 0) {
        buffer_put(ascii);
    }
}

/* IRQ1 interrupt wrapper */
static void keyboard_irq_handler(struct interrupt_frame *frame) {
    (void)frame;  /* Unused */
    keyboard_interrupt_handler();
    pic_send_eoi(1);  /* Send EOI to PIC for IRQ1 */
}

/* Initialize keyboard */
void keyboard_init(void) {
    shift_pressed = false;
    ctrl_pressed = false;
    alt_pressed = false;
    current_layout = KEYBOARD_LAYOUT_QWERTY;

    /* Clear buffer */
    buffer_read_pos = 0;
    buffer_write_pos = 0;
}

/* Enable keyboard interrupts */
void keyboard_enable_interrupts(void) {
    /* Register IRQ1 handler */
    idt_register_handler(IRQ1, keyboard_irq_handler);

    /* Unmask IRQ1 on PIC */
    pic_unmask_irq(1);
}

/* Disable keyboard interrupts */
void keyboard_disable_interrupts(void) {
    /* Mask IRQ1 on PIC */
    pic_mask_irq(1);

    /* Unregister handler */
    idt_unregister_handler(IRQ1);
}

/* Check if key is available */
bool keyboard_haskey(void) {
    return !buffer_is_empty();
}

/* Get keyboard input (non-blocking) */
int keyboard_getchar(void) {
    return buffer_get();
}

/* Set keyboard layout */
void keyboard_set_layout(keyboard_layout_t layout) {
    current_layout = layout;
}

/* Get current keyboard layout */
keyboard_layout_t keyboard_get_layout(void) {
    return current_layout;
}

/* Get line of input (blocking, waits for Enter) - BONUS */
int keyboard_getline(char *buffer, int max_len) {
    if (!buffer || max_len <= 0) {
        return -1;
    }

    int pos = 0;

    while (1) {
        /* Wait for a key */
        while (!keyboard_haskey()) {
            __asm__ volatile("hlt");  /* Wait for interrupt */
        }

        int c = keyboard_getchar();

        if (c == '\n' || c == '\r') {
            /* Enter pressed - end input */
            buffer[pos] = '\0';
            printk("\n");
            return pos;
        } else if (c == '\b' || c == 0x7F) {
            /* Backspace */
            if (pos > 0) {
                pos--;
                printk("\b \b");  /* Erase character on screen */
            }
        } else if (c >= 32 && c < 127) {
            /* Printable character */
            if (pos < max_len - 1) {
                buffer[pos++] = (char)c;
                printk("%c", c);  /* Echo character */
            }
        }
    }
}
