/* login.h - Login system for KFS-7 */

#ifndef LOGIN_H
#define LOGIN_H

#include "types.h"

/* Login attempt result */
typedef enum {
    LOGIN_SUCCESS = 0,
    LOGIN_INVALID_USER,
    LOGIN_INVALID_PASSWORD,
    LOGIN_SYSTEM_ERROR
} login_result_t;

/* Initialize login system */
void login_init(void);

/* Display login prompt */
void login_prompt(void);

/* Process login attempt */
login_result_t login_attempt(const char *username, const char *password);

/* Logout current user */
void login_logout(void);

/* Interactive login (reads username and password) */
int login_interactive(void);

/* Get input with echo (for username) */
int login_get_input(char *buf, size_t buf_size, int echo);

#endif /* LOGIN_H */
