/* login.c - Login system implementation */

#include "../include/login.h"
#include "../include/printf.h"
#include "../include/string.h"
#include "../include/user.h"
#include "../include/keyboard.h"
#include "../include/tty.h"

/* Login system state */
static int login_initialized = 0;

/* Initialize login system */
void login_init(void) {
    login_initialized = 1;
    printk("[LOGIN] Login system initialized\n");
}

/* Display login prompt */
void login_prompt(void) {
    printk("\n");
    printk("================================================================================\n");
    printk("                    KFS-7 Operating System - Login\n");
    printk("================================================================================\n");
    printk("\n");
    printk("Default accounts:\n");
    printk("  Username: root     Password: root\n");
    printk("  Username: user     Password: user\n");
    printk("\n");
}

/* Get input with optional echo */
int login_get_input(char *buf, size_t buf_size, int echo) {
    if (!buf || buf_size == 0) {
        return -1;
    }

    size_t pos = 0;
    buf[0] = '\0';

    while (1) {
        /* Wait for keyboard input */
        char c = keyboard_getchar();

        if (c == 0) {
            /* No input available, wait */
            continue;
        }

        if (c == '\n') {
            /* Enter pressed */
            buf[pos] = '\0';
            printk("\n");
            return (int)pos;
        } else if (c == '\b') {
            /* Backspace */
            if (pos > 0) {
                pos--;
                buf[pos] = '\0';
                if (echo) {
                    printk("\b \b");  /* Erase character on screen */
                }
            }
        } else if (c >= 32 && c < 127) {
            /* Printable character */
            if (pos < buf_size - 1) {
                buf[pos++] = c;
                buf[pos] = '\0';
                if (echo) {
                    printk("%c", c);
                } else {
                    printk("*");  /* Show asterisk for password */
                }
            }
        }
    }

    return (int)pos;
}

/* Process login attempt */
login_result_t login_attempt(const char *username, const char *password) {
    if (!username || !password) {
        return LOGIN_SYSTEM_ERROR;
    }

    /* Check if user exists */
    user_account_t *account = user_get_by_name(username);
    if (!account) {
        return LOGIN_INVALID_USER;
    }

    /* Verify password */
    if (!user_verify_password(username, password)) {
        return LOGIN_INVALID_PASSWORD;
    }

    /* Login successful */
    if (user_login(username, password) < 0) {
        return LOGIN_SYSTEM_ERROR;
    }

    return LOGIN_SUCCESS;
}

/* Interactive login */
int login_interactive(void) {
    char username[64];
    char password[64];
    int attempts = 0;
    const int max_attempts = 3;

    while (attempts < max_attempts) {
        /* Show login prompt */
        login_prompt();

        /* Get username */
        printk("Username: ");
        if (login_get_input(username, sizeof(username), 1) < 0) {
            attempts++;
            continue;
        }

        /* Get password */
        printk("Password: ");
        if (login_get_input(password, sizeof(password), 0) < 0) {
            attempts++;
            continue;
        }

        /* Attempt login */
        login_result_t result = login_attempt(username, password);

        switch (result) {
            case LOGIN_SUCCESS:
                printk("\n");
                printk("[LOGIN] Login successful! Welcome, %s\n", username);
                printk("\n");

                /* Set user for current TTY */
                user_account_t *account = user_get_by_name(username);
                if (account) {
                    int tty_num = tty_get_active();
                    tty_set_user(tty_num, account->uid, username);
                }

                return 0;

            case LOGIN_INVALID_USER:
                printk("\n[LOGIN] Invalid username\n");
                attempts++;
                break;

            case LOGIN_INVALID_PASSWORD:
                printk("\n[LOGIN] Invalid password\n");
                attempts++;
                break;

            case LOGIN_SYSTEM_ERROR:
                printk("\n[LOGIN] System error\n");
                attempts++;
                break;
        }

        printk("\n");
    }

    printk("[LOGIN] Too many failed login attempts\n");
    return -1;
}

/* Logout current user */
void login_logout(void) {
    int tty_num = tty_get_active();
    tty_clear_user(tty_num);
    printk("[LOGIN] Logged out\n");
}
