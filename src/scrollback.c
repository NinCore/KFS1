/* scrollback.c - Screen scrollback buffer implementation */

#include "../include/scrollback.h"
#include "../include/vga.h"
#include "../include/string.h"

#define VGA_MEMORY 0xB8000

/* Scrollback buffer - circular buffer of screen lines */
static uint16_t scrollback_buffer[SCROLLBACK_LINES][VGA_WIDTH];
static int scrollback_head = 0;  /* Next position to write */
static int scrollback_count = 0; /* Number of lines stored */
static int scrollback_offset = 0; /* Current scroll offset (0 = bottom/current) */
static uint16_t saved_screen[VGA_HEIGHT][VGA_WIDTH]; /* Save current screen when scrolling */
static bool is_scrolled = false;

void scrollback_init(void) {
    /* Clear the scrollback buffer */
    uint16_t blank = ' ' | (0x07 << 8);
    for (int i = 0; i < SCROLLBACK_LINES; i++) {
        for (int j = 0; j < VGA_WIDTH; j++) {
            scrollback_buffer[i][j] = blank;
        }
    }
    scrollback_head = 0;
    scrollback_count = 0;
    scrollback_offset = 0;
    is_scrolled = false;
}

void scrollback_add_line(const uint16_t *line_data) {
    /* Only add lines when not scrolled (at bottom) */
    if (is_scrolled) {
        return;
    }

    /* Copy line to buffer */
    for (int i = 0; i < VGA_WIDTH; i++) {
        scrollback_buffer[scrollback_head][i] = line_data[i];
    }

    /* Move head forward */
    scrollback_head = (scrollback_head + 1) % SCROLLBACK_LINES;

    /* Increment count if not full */
    if (scrollback_count < SCROLLBACK_LINES) {
        scrollback_count++;
    }
}

void scrollback_scroll(int delta) {
    if (delta == 0) {
        return;
    }

    /* Save current screen if just starting to scroll */
    if (!is_scrolled && delta > 0) {
        uint16_t *vga = (uint16_t*)VGA_MEMORY;
        for (int row = 0; row < VGA_HEIGHT; row++) {
            for (int col = 0; col < VGA_WIDTH; col++) {
                saved_screen[row][col] = vga[row * VGA_WIDTH + col];
            }
        }
        is_scrolled = true;
    }

    /* Update offset */
    scrollback_offset += delta;

    /* Clamp offset */
    int max_offset = scrollback_count - VGA_HEIGHT;
    if (max_offset < 0) max_offset = 0;

    if (scrollback_offset < 0) {
        scrollback_offset = 0;
        /* Restore original screen */
        if (delta < 0) {
            is_scrolled = false;
            scrollback_reset();
        }
    } else if (scrollback_offset > max_offset) {
        scrollback_offset = max_offset;
    }

    /* Render scrollback view */
    if (is_scrolled && scrollback_offset == 0) {
        /* Back at bottom - restore saved screen */
        is_scrolled = false;
        uint16_t *vga = (uint16_t*)VGA_MEMORY;
        for (int row = 0; row < VGA_HEIGHT; row++) {
            for (int col = 0; col < VGA_WIDTH; col++) {
                vga[row * VGA_WIDTH + col] = saved_screen[row][col];
            }
        }
    } else if (is_scrolled) {
        /* Calculate which lines to display */
        int start_line = scrollback_count - VGA_HEIGHT - scrollback_offset;
        if (start_line < 0) start_line = 0;

        uint16_t *vga = (uint16_t*)VGA_MEMORY;
        for (int row = 0; row < VGA_HEIGHT; row++) {
            int buffer_index = (start_line + row) % SCROLLBACK_LINES;
            if (start_line + row < scrollback_count) {
                for (int col = 0; col < VGA_WIDTH; col++) {
                    vga[row * VGA_WIDTH + col] = scrollback_buffer[buffer_index][col];
                }
            } else {
                /* Empty line */
                uint16_t blank = ' ' | (0x07 << 8);
                for (int col = 0; col < VGA_WIDTH; col++) {
                    vga[row * VGA_WIDTH + col] = blank;
                }
            }
        }

        /* Show scroll indicator in top-right corner */
        const char *indicator = "[SCROLL]";
        uint16_t attr = (VGA_COLOR_BLACK << 4) | VGA_COLOR_LIGHT_BROWN;
        for (int i = 0; indicator[i] && i < 8; i++) {
            vga[VGA_WIDTH - 8 + i] = indicator[i] | (attr << 8);
        }
    }
}

int scrollback_get_offset(void) {
    return scrollback_offset;
}

void scrollback_reset(void) {
    if (is_scrolled) {
        uint16_t *vga = (uint16_t*)VGA_MEMORY;
        for (int row = 0; row < VGA_HEIGHT; row++) {
            for (int col = 0; col < VGA_WIDTH; col++) {
                vga[row * VGA_WIDTH + col] = saved_screen[row][col];
            }
        }
        is_scrolled = false;
    }
    scrollback_offset = 0;
}

bool scrollback_is_scrolled(void) {
    return is_scrolled;
}
