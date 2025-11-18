/* user.h - User accounts and password management for KFS-7 */

#ifndef USER_H
#define USER_H

#include "types.h"

/* Maximum number of user accounts */
#define MAX_USERS 32

/* Maximum lengths */
#define USER_NAME_MAX 32
#define USER_PASSWORD_HASH_MAX 64
#define USER_HOME_MAX 64
#define USER_SHELL_MAX 64

/* User account structure (/etc/passwd) */
typedef struct user_account {
    char username[USER_NAME_MAX];
    uint32_t uid;
    uint32_t gid;
    char home[USER_HOME_MAX];
    char shell[USER_SHELL_MAX];
    int in_use;
} user_account_t;

/* Password entry structure (/etc/shadow) */
typedef struct password_entry {
    char username[USER_NAME_MAX];
    char password_hash[USER_PASSWORD_HASH_MAX];
    int in_use;
} password_entry_t;

/* User database */
typedef struct user_db {
    user_account_t accounts[MAX_USERS];
    password_entry_t passwords[MAX_USERS];
    int user_count;
} user_db_t;

/* Initialize user system */
void user_init(void);

/* User management functions */
int user_create(const char *username, const char *password, uint32_t uid, uint32_t gid,
                const char *home, const char *shell);
int user_delete(const char *username);
user_account_t *user_get_by_name(const char *username);
user_account_t *user_get_by_uid(uint32_t uid);

/* Password management */
int user_set_password(const char *username, const char *password);
int user_verify_password(const char *username, const char *password);
void user_hash_password(const char *password, char *hash_out, size_t hash_size);

/* Login system */
int user_login(const char *username, const char *password);
uint32_t user_get_current_uid(void);
const char *user_get_current_username(void);

/* Print user database (for debugging) */
void user_print_accounts(void);

/* System calls */
int sys_getuid(uint32_t unused1, uint32_t unused2, uint32_t unused3, uint32_t unused4, uint32_t unused5);
int sys_setuid(uint32_t uid, uint32_t unused1, uint32_t unused2, uint32_t unused3, uint32_t unused4);

#endif /* USER_H */
