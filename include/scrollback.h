/* scrollback.h - Screen scrollback buffer for viewing history */

#ifndef SCROLLBACK_H
#define SCROLLBACK_H

#include "types.h"
#include "vga.h"

/* Number of lines to keep in scrollback buffer */
#define SCROLLBACK_LINES 200

/* Initialize scrollback buffer */
void scrollback_init(void);

/* Add a line to the scrollback buffer */
void scrollback_add_line(const uint16_t *line_data);

/* Scroll up by N lines (positive = up, negative = down) */
void scrollback_scroll(int delta);

/* Get current scroll offset */
int scrollback_get_offset(void);

/* Reset scroll to bottom (current output) */
void scrollback_reset(void);

/* Check if we're currently scrolled up */
bool scrollback_is_scrolled(void);

#endif /* SCROLLBACK_H */
