/* vga.c - VGA text mode driver implementation */

#include "../include/vga.h"
#include "../include/string.h"
#include "../include/scrollback.h"

/* VGA buffer address */
#define VGA_MEMORY 0xB8000

/* VGA hardware ports */
#define VGA_CTRL_REGISTER 0x3D4
#define VGA_DATA_REGISTER 0x3D5

/* Port I/O functions */
static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

/* Current position and color */
static uint16_t *vga_buffer;
static size_t vga_row;
static size_t vga_column;
static uint8_t vga_color;

/* Create VGA color byte */
static inline uint8_t vga_entry_color(vga_color_t fg, vga_color_t bg) {
    return fg | bg << 4;
}

/* Create VGA entry (character + color) */
static inline uint16_t vga_entry(unsigned char c, uint8_t color) {
    return (uint16_t)c | (uint16_t)color << 8;
}

void vga_init(void) {
    vga_buffer = (uint16_t*)VGA_MEMORY;
    vga_row = 0;
    vga_column = 0;
    vga_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_clear();
    vga_enable_cursor(0, 15);  /* Enable cursor */
    vga_update_cursor();
}

void vga_clear(void) {
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            vga_buffer[index] = vga_entry(' ', vga_color);
        }
    }
    vga_row = 0;
    vga_column = 0;
}

void vga_set_color(vga_color_t fg, vga_color_t bg) {
    vga_color = vga_entry_color(fg, bg);
}

/* Put character at specific position */
static void vga_putentryat(char c, uint8_t color, size_t x, size_t y) {
    const size_t index = y * VGA_WIDTH + x;
    vga_buffer[index] = vga_entry(c, color);
}

/* Scroll screen up one line */
static void vga_scroll(void) {
    /* Save the top line to scrollback buffer before it's lost */
    uint16_t top_line[VGA_WIDTH];
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        top_line[x] = vga_buffer[x];
    }
    scrollback_add_line(top_line);

    /* Move all lines up */
    for (size_t y = 0; y < VGA_HEIGHT - 1; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t src_index = (y + 1) * VGA_WIDTH + x;
            const size_t dst_index = y * VGA_WIDTH + x;
            vga_buffer[dst_index] = vga_buffer[src_index];
        }
    }

    /* Clear last line */
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        const size_t index = (VGA_HEIGHT - 1) * VGA_WIDTH + x;
        vga_buffer[index] = vga_entry(' ', vga_color);
    }
}

void vga_putchar(char c) {
    if (c == '\n') {
        vga_column = 0;
        vga_row++;
    } else if (c == '\r') {
        vga_column = 0;
    } else if (c == '\t') {
        vga_column = (vga_column + 4) & ~(4 - 1);
    } else {
        vga_putentryat(c, vga_color, vga_column, vga_row);
        vga_column++;
    }

    if (vga_column >= VGA_WIDTH) {
        vga_column = 0;
        vga_row++;
    }

    if (vga_row >= VGA_HEIGHT) {
        vga_scroll();
        vga_row = VGA_HEIGHT - 1;
    }

    vga_update_cursor();
}

void vga_print(const char *str) {
    size_t i = 0;
    while (str[i]) {
        vga_putchar(str[i]);
        i++;
    }
}

/* Cursor management functions */
void vga_enable_cursor(uint8_t cursor_start, uint8_t cursor_end) {
    outb(VGA_CTRL_REGISTER, 0x0A);
    outb(VGA_DATA_REGISTER, (inb(VGA_DATA_REGISTER) & 0xC0) | cursor_start);
    outb(VGA_CTRL_REGISTER, 0x0B);
    outb(VGA_DATA_REGISTER, (inb(VGA_DATA_REGISTER) & 0xE0) | cursor_end);
}

void vga_disable_cursor(void) {
    outb(VGA_CTRL_REGISTER, 0x0A);
    outb(VGA_DATA_REGISTER, 0x20);
}

void vga_update_cursor(void) {
    uint16_t pos = vga_row * VGA_WIDTH + vga_column;
    outb(VGA_CTRL_REGISTER, 0x0F);
    outb(VGA_DATA_REGISTER, (uint8_t)(pos & 0xFF));
    outb(VGA_CTRL_REGISTER, 0x0E);
    outb(VGA_DATA_REGISTER, (uint8_t)((pos >> 8) & 0xFF));
}

void vga_get_cursor_position(size_t *row, size_t *col) {
    if (row) *row = vga_row;
    if (col) *col = vga_column;
}

void vga_set_cursor_position(size_t row, size_t col) {
    if (row < VGA_HEIGHT && col < VGA_WIDTH) {
        vga_row = row;
        vga_column = col;
        vga_update_cursor();
    }
}
