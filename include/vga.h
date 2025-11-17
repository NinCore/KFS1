/* vga.h - VGA text mode driver */

#ifndef VGA_H
#define VGA_H

#include "types.h"

/* VGA color codes */
typedef enum {
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_BLUE = 1,
    VGA_COLOR_GREEN = 2,
    VGA_COLOR_CYAN = 3,
    VGA_COLOR_RED = 4,
    VGA_COLOR_MAGENTA = 5,
    VGA_COLOR_BROWN = 6,
    VGA_COLOR_LIGHT_GREY = 7,
    VGA_COLOR_DARK_GREY = 8,
    VGA_COLOR_LIGHT_BLUE = 9,
    VGA_COLOR_LIGHT_GREEN = 10,
    VGA_COLOR_LIGHT_CYAN = 11,
    VGA_COLOR_LIGHT_RED = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_LIGHT_BROWN = 14,
    VGA_COLOR_WHITE = 15,
} vga_color_t;

/* VGA constants */
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

/* Initialize VGA */
void vga_init(void);

/* Clear screen */
void vga_clear(void);

/* Print character */
void vga_putchar(char c);

/* Print string */
void vga_print(const char *str);

/* Set color */
void vga_set_color(vga_color_t fg, vga_color_t bg);

#endif /* VGA_H */
