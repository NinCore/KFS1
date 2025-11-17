/* keyboard.h - Keyboard driver */

#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "types.h"

/* Special keys */
#define KEY_F1 0x80
#define KEY_F2 0x81
#define KEY_F3 0x82
#define KEY_F4 0x83
#define KEY_F5 0x84
#define KEY_F6 0x85
#define KEY_F7 0x86
#define KEY_F8 0x87
#define KEY_F9 0x88
#define KEY_F10 0x89
#define KEY_F11 0x8A
#define KEY_F12 0x8B

#define KEY_ESC 0x1B
#define KEY_BACKSPACE 0x08
#define KEY_TAB 0x09
#define KEY_ENTER 0x0A

/* Keyboard initialization */
void keyboard_init(void);

/* Get keyboard input (non-blocking) - returns 0 if no key, or char/special key code */
int keyboard_getchar(void);

/* Check if key is available */
bool keyboard_haskey(void);

#endif /* KEYBOARD_H */
