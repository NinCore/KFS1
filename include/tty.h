/* tty.h - Multiple TTY (terminal) support for KFS-7 BONUS */

#ifndef TTY_H
#define TTY_H

#include "types.h"
#include "vga.h"

/* Number of virtual terminals */
#define TTY_COUNT 4

/* TTY buffer size */
#define TTY_BUFFER_SIZE 4096
#define TTY_HISTORY_SIZE 2000

/* TTY state */
typedef struct tty {
    int tty_num;
    int active;

    /* VGA buffer for this TTY */
    uint16_t buffer[VGA_WIDTH * VGA_HEIGHT];

    /* Cursor position */
    int cursor_x;
    int cursor_y;

    /* Color */
    uint8_t fg_color;
    uint8_t bg_color;

    /* Input buffer */
    char input_buffer[TTY_BUFFER_SIZE];
    int input_head;
    int input_tail;
    int input_count;

    /* Process attached to this TTY */
    uint32_t attached_pid;

    /* User logged in to this TTY */
    uint32_t logged_in_uid;
    char logged_in_user[32];
    int login_required;  /* 1 if login prompt should be shown */

} tty_t;

/* TTY management */
void tty_init(void);
void tty_switch(int tty_num);
int tty_get_active(void);
tty_t *tty_get(int tty_num);
tty_t *tty_get_current(void);

/* TTY output operations */
void tty_putchar(int tty_num, char c);
void tty_write(int tty_num, const char *str, size_t len);
void tty_clear(int tty_num);
void tty_set_color(int tty_num, uint8_t fg, uint8_t bg);

/* TTY input operations */
void tty_put_input(int tty_num, char c);
int tty_read_input(int tty_num, char *buf, size_t len);

/* TTY cursor operations */
void tty_update_cursor(int tty_num);
void tty_set_cursor(int tty_num, int x, int y);

/* TTY user session */
void tty_set_user(int tty_num, uint32_t uid, const char *username);
void tty_clear_user(int tty_num);

#endif /* TTY_H */
