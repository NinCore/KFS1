/* screen.c - Multiple virtual screens implementation */

#include "../include/screen.h"
#include "../include/vga.h"
#include "../include/string.h"

#define VGA_MEMORY 0xB8000

/* Virtual screen buffers */
static uint16_t screen_buffers[MAX_SCREENS][VGA_WIDTH * VGA_HEIGHT];
static int current_screen = 0;
static size_t screen_cursors[MAX_SCREENS][2];  /* [row, col] for each screen */

void screen_init(void) {
    /* Initialize all screen buffers to empty */
    for (int i = 0; i < MAX_SCREENS; i++) {
        uint16_t blank = ' ' | (0x07 << 8);  /* Space with light grey on black */
        for (int j = 0; j < VGA_WIDTH * VGA_HEIGHT; j++) {
            screen_buffers[i][j] = blank;
        }
        screen_cursors[i][0] = 0;  /* row */
        screen_cursors[i][1] = 0;  /* col */
    }
    current_screen = 0;

    /* Save initial VGA buffer to screen 0 */
    uint16_t *vga = (uint16_t*)VGA_MEMORY;
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        screen_buffers[0][i] = vga[i];
    }
}

void screen_save(void) {
    /* Save current VGA buffer to current screen buffer */
    uint16_t *vga = (uint16_t*)VGA_MEMORY;
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        screen_buffers[current_screen][i] = vga[i];
    }

    /* Save cursor position */
    vga_get_cursor_position(&screen_cursors[current_screen][0],
                           &screen_cursors[current_screen][1]);
}

void screen_restore(int screen_num) {
    if (screen_num < 0 || screen_num >= MAX_SCREENS) {
        return;
    }

    /* Restore screen buffer to VGA memory */
    uint16_t *vga = (uint16_t*)VGA_MEMORY;
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga[i] = screen_buffers[screen_num][i];
    }

    /* Restore cursor position */
    vga_set_cursor_position(screen_cursors[screen_num][0],
                           screen_cursors[screen_num][1]);
}

void screen_switch(int screen_num) {
    if (screen_num < 0 || screen_num >= MAX_SCREENS || screen_num == current_screen) {
        return;
    }

    /* Save current screen */
    screen_save();

    /* Switch to new screen */
    current_screen = screen_num;

    /* Restore new screen */
    screen_restore(screen_num);
}

int screen_get_current(void) {
    return current_screen;
}
