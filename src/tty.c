/* tty.c - Multiple TTY implementation */

#include "../include/tty.h"
#include "../include/printf.h"
#include "../include/string.h"
#include "../include/vga.h"
#include "../include/io.h"

/* TTY array */
static tty_t ttys[TTY_COUNT];

/* Current active TTY */
static int active_tty = 0;

/* Initialize TTY system */
void tty_init(void) {
    /* Initialize all TTYs */
    for (int i = 0; i < TTY_COUNT; i++) {
        memset(&ttys[i], 0, sizeof(tty_t));
        ttys[i].tty_num = i;
        ttys[i].active = (i == 0);
        ttys[i].fg_color = VGA_COLOR_LIGHT_GREY;
        ttys[i].bg_color = VGA_COLOR_BLACK;
        ttys[i].cursor_x = 0;
        ttys[i].cursor_y = 0;
        ttys[i].logged_in_uid = 0xFFFFFFFF;  /* Not logged in */

        /* Initialize buffer with blank spaces */
        uint16_t blank = vga_make_entry(' ', vga_make_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
        for (int j = 0; j < VGA_WIDTH * VGA_HEIGHT; j++) {
            ttys[i].buffer[j] = blank;
        }

        /* Write welcome message to each TTY */
        /* Create welcome message manually (no snprintf in kernel) */
        char welcome[128] = "KFS-7 Operating System - TTY ";
        welcome[29] = '0' + i;
        welcome[30] = '\n';
        welcome[31] = '\n';
        welcome[32] = '\0';

        /* Write welcome message to buffer */
        const char *ptr = welcome;
        int x = 0, y = 0;
        while (*ptr) {
            if (*ptr == '\n') {
                x = 0;
                y++;
            } else {
                uint16_t entry = vga_make_entry(*ptr, vga_make_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
                ttys[i].buffer[y * VGA_WIDTH + x] = entry;
                x++;
            }
            ptr++;
        }

        ttys[i].cursor_x = 0;
        ttys[i].cursor_y = y + 1;
    }

    /* Set TTY 0 as active and copy to VGA buffer */
    active_tty = 0;
    uint16_t *vga_buffer = (uint16_t *)VGA_MEMORY;
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_buffer[i] = ttys[0].buffer[i];
    }

    tty_update_cursor(0);

    printk("[TTY] TTY system initialized (%d terminals)\n", TTY_COUNT);
}

/* Get active TTY number */
int tty_get_active(void) {
    return active_tty;
}

/* Get TTY by number */
tty_t *tty_get(int tty_num) {
    if (tty_num < 0 || tty_num >= TTY_COUNT) {
        return NULL;
    }
    return &ttys[tty_num];
}

/* Get current active TTY */
tty_t *tty_get_current(void) {
    return &ttys[active_tty];
}

/* Switch to different TTY */
void tty_switch(int tty_num) {
    if (tty_num < 0 || tty_num >= TTY_COUNT || tty_num == active_tty) {
        return;
    }

    /* Save current VGA buffer to current TTY */
    uint16_t *vga_buffer = (uint16_t *)VGA_MEMORY;
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        ttys[active_tty].buffer[i] = vga_buffer[i];
    }

    /* Switch active TTY */
    ttys[active_tty].active = 0;
    active_tty = tty_num;
    ttys[active_tty].active = 1;

    /* Restore new TTY buffer to VGA */
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_buffer[i] = ttys[active_tty].buffer[i];
    }

    /* Update cursor */
    tty_update_cursor(active_tty);

    /* Print notification at bottom of screen */
    char msg[32] = "[ Switched to TTY ";
    msg[18] = '0' + tty_num;
    msg[19] = ' ';
    msg[20] = ']';
    msg[21] = '\0';
    int len = strlen(msg);
    int start_x = (VGA_WIDTH - len) / 2;

    for (int i = 0; i < len; i++) {
        uint16_t entry = vga_make_entry(msg[i], vga_make_color(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_CYAN));
        vga_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + start_x + i] = entry;
    }
}

/* Update hardware cursor */
void tty_update_cursor(int tty_num) {
    if (tty_num != active_tty) {
        return;  /* Only update cursor for active TTY */
    }

    tty_t *tty = &ttys[tty_num];
    uint16_t pos = tty->cursor_y * VGA_WIDTH + tty->cursor_x;

    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

/* Set cursor position */
void tty_set_cursor(int tty_num, int x, int y) {
    if (tty_num < 0 || tty_num >= TTY_COUNT) {
        return;
    }

    tty_t *tty = &ttys[tty_num];
    tty->cursor_x = x;
    tty->cursor_y = y;

    if (tty_num == active_tty) {
        tty_update_cursor(tty_num);
    }
}

/* Scroll TTY buffer up by one line */
static void tty_scroll(tty_t *tty) {
    /* Move all lines up */
    for (int y = 0; y < VGA_HEIGHT - 1; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            tty->buffer[y * VGA_WIDTH + x] = tty->buffer[(y + 1) * VGA_WIDTH + x];
        }
    }

    /* Clear bottom line */
    uint16_t blank = vga_make_entry(' ', vga_make_color(tty->fg_color, tty->bg_color));
    for (int x = 0; x < VGA_WIDTH; x++) {
        tty->buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = blank;
    }

    /* If this is the active TTY, update VGA buffer */
    if (tty->active) {
        uint16_t *vga_buffer = (uint16_t *)VGA_MEMORY;
        for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
            vga_buffer[i] = tty->buffer[i];
        }
    }
}

/* Put character to TTY */
void tty_putchar(int tty_num, char c) {
    if (tty_num < 0 || tty_num >= TTY_COUNT) {
        return;
    }

    tty_t *tty = &ttys[tty_num];

    /* Handle special characters */
    if (c == '\n') {
        tty->cursor_x = 0;
        tty->cursor_y++;
    } else if (c == '\r') {
        tty->cursor_x = 0;
    } else if (c == '\t') {
        tty->cursor_x = (tty->cursor_x + 4) & ~3;
    } else if (c == '\b') {
        if (tty->cursor_x > 0) {
            tty->cursor_x--;
        }
    } else {
        /* Regular character */
        uint16_t entry = vga_make_entry(c, vga_make_color(tty->fg_color, tty->bg_color));
        tty->buffer[tty->cursor_y * VGA_WIDTH + tty->cursor_x] = entry;

        /* Update VGA if active */
        if (tty->active) {
            uint16_t *vga_buffer = (uint16_t *)VGA_MEMORY;
            vga_buffer[tty->cursor_y * VGA_WIDTH + tty->cursor_x] = entry;
        }

        tty->cursor_x++;
    }

    /* Wrap to next line */
    if (tty->cursor_x >= VGA_WIDTH) {
        tty->cursor_x = 0;
        tty->cursor_y++;
    }

    /* Scroll if necessary */
    if (tty->cursor_y >= VGA_HEIGHT) {
        tty_scroll(tty);
        tty->cursor_y = VGA_HEIGHT - 1;
    }

    /* Update cursor */
    if (tty->active) {
        tty_update_cursor(tty_num);
    }
}

/* Write string to TTY */
void tty_write(int tty_num, const char *str, size_t len) {
    for (size_t i = 0; i < len; i++) {
        tty_putchar(tty_num, str[i]);
    }
}

/* Clear TTY */
void tty_clear(int tty_num) {
    if (tty_num < 0 || tty_num >= TTY_COUNT) {
        return;
    }

    tty_t *tty = &ttys[tty_num];

    /* Clear buffer */
    uint16_t blank = vga_make_entry(' ', vga_make_color(tty->fg_color, tty->bg_color));
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        tty->buffer[i] = blank;
    }

    /* Reset cursor */
    tty->cursor_x = 0;
    tty->cursor_y = 0;

    /* Update VGA if active */
    if (tty->active) {
        uint16_t *vga_buffer = (uint16_t *)VGA_MEMORY;
        for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
            vga_buffer[i] = blank;
        }
        tty_update_cursor(tty_num);
    }
}

/* Set TTY color */
void tty_set_color(int tty_num, uint8_t fg, uint8_t bg) {
    if (tty_num < 0 || tty_num >= TTY_COUNT) {
        return;
    }

    ttys[tty_num].fg_color = fg;
    ttys[tty_num].bg_color = bg;
}

/* Put character to input buffer */
void tty_put_input(int tty_num, char c) {
    if (tty_num < 0 || tty_num >= TTY_COUNT) {
        return;
    }

    tty_t *tty = &ttys[tty_num];

    if (tty->input_count < TTY_BUFFER_SIZE) {
        tty->input_buffer[tty->input_tail] = c;
        tty->input_tail = (tty->input_tail + 1) % TTY_BUFFER_SIZE;
        tty->input_count++;
    }
}

/* Read from input buffer */
int tty_read_input(int tty_num, char *buf, size_t len) {
    if (tty_num < 0 || tty_num >= TTY_COUNT || !buf) {
        return -1;
    }

    tty_t *tty = &ttys[tty_num];
    int count = 0;

    while (count < (int)len && tty->input_count > 0) {
        buf[count++] = tty->input_buffer[tty->input_head];
        tty->input_head = (tty->input_head + 1) % TTY_BUFFER_SIZE;
        tty->input_count--;
    }

    return count;
}

/* Set user for TTY */
void tty_set_user(int tty_num, uint32_t uid, const char *username) {
    if (tty_num < 0 || tty_num >= TTY_COUNT) {
        return;
    }

    tty_t *tty = &ttys[tty_num];
    tty->logged_in_uid = uid;
    if (username) {
        strncpy(tty->logged_in_user, username, sizeof(tty->logged_in_user) - 1);
        tty->logged_in_user[sizeof(tty->logged_in_user) - 1] = '\0';
    }
}

/* Clear user from TTY */
void tty_clear_user(int tty_num) {
    if (tty_num < 0 || tty_num >= TTY_COUNT) {
        return;
    }

    ttys[tty_num].logged_in_uid = 0xFFFFFFFF;
    ttys[tty_num].logged_in_user[0] = '\0';
}
