/* screen.h - Multiple virtual screens management */

#ifndef SCREEN_H
#define SCREEN_H

#include "types.h"
#include "vga.h"

#define MAX_SCREENS 4
#define SCREEN_BUFFER_SIZE (VGA_WIDTH * VGA_HEIGHT * 2)

/* Initialize screen system */
void screen_init(void);

/* Switch to a specific screen (0-3) */
void screen_switch(int screen_num);

/* Get current active screen */
int screen_get_current(void);

/* Save current screen state */
void screen_save(void);

/* Restore screen state */
void screen_restore(int screen_num);

#endif /* SCREEN_H */
