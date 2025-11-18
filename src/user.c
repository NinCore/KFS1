/* user.c - User accounts and password management implementation */

#include "../include/user.h"
#include "../include/printf.h"
#include "../include/string.h"
#include "../include/kmalloc.h"
#include "../include/process.h"

/* User database */
static user_db_t user_db;

/* Current logged in user */
static uint32_t current_uid = 0;  /* Default to root */
static char current_username[USER_NAME_MAX] = "root";

/* Simple hash function for passwords (NOT secure, just for demonstration) */
static uint32_t simple_hash(const char *str) {
    uint32_t hash = 5381;
    int c;

    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }

    return hash;
}

/* Hash password (using simple hash for now) */
void user_hash_password(const char *password, char *hash_out, size_t hash_size) {
    if (!password || !hash_out || hash_size < USER_PASSWORD_HASH_MAX) {
        return;
    }

    /* Simple hash (NOT cryptographically secure!) */
    /* In a real system, use bcrypt, scrypt, or argon2 */
    uint32_t hash1 = simple_hash(password);
    uint32_t hash2 = simple_hash(password + strlen(password) / 2);

    /* Create hex string manually (no snprintf in kernel) */
    const char hex[] = "0123456789abcdef";
    int pos = 0;
    for (int i = 7; i >= 0; i--) {
        hash_out[pos++] = hex[(hash1 >> (i * 4)) & 0xF];
    }
    for (int i = 7; i >= 0; i--) {
        hash_out[pos++] = hex[(hash2 >> (i * 4)) & 0xF];
    }
    hash_out[pos] = '\0';
}

/* Initialize user system with default users */
void user_init(void) {
    memset(&user_db, 0, sizeof(user_db_t));
    user_db.user_count = 0;

    /* Create root user (uid=0) */
    user_create("root", "root", 0, 0, "/root", "/bin/sh");

    /* Create default user (uid=1000) */
    user_create("user", "user", 1000, 1000, "/home/user", "/bin/sh");

    printk("[USER] User account system initialized\n");
    printk("[USER] Default users created: root (uid=0), user (uid=1000)\n");
}

/* Create a new user account */
int user_create(const char *username, const char *password, uint32_t uid, uint32_t gid,
                const char *home, const char *shell) {
    if (!username || !password || user_db.user_count >= MAX_USERS) {
        return -1;
    }

    /* Check if user already exists */
    if (user_get_by_name(username) || user_get_by_uid(uid)) {
        return -1;  /* User already exists */
    }

    /* Find free slot */
    int slot = -1;
    for (int i = 0; i < MAX_USERS; i++) {
        if (!user_db.accounts[i].in_use) {
            slot = i;
            break;
        }
    }

    if (slot == -1) {
        return -1;  /* No free slots */
    }

    /* Create user account */
    user_account_t *account = &user_db.accounts[slot];
    strncpy(account->username, username, USER_NAME_MAX - 1);
    account->username[USER_NAME_MAX - 1] = '\0';
    account->uid = uid;
    account->gid = gid;
    strncpy(account->home, home, USER_HOME_MAX - 1);
    account->home[USER_HOME_MAX - 1] = '\0';
    strncpy(account->shell, shell, USER_SHELL_MAX - 1);
    account->shell[USER_SHELL_MAX - 1] = '\0';
    account->in_use = 1;

    /* Create password entry */
    password_entry_t *pass_entry = &user_db.passwords[slot];
    strncpy(pass_entry->username, username, USER_NAME_MAX - 1);
    pass_entry->username[USER_NAME_MAX - 1] = '\0';
    user_hash_password(password, pass_entry->password_hash, USER_PASSWORD_HASH_MAX);
    pass_entry->in_use = 1;

    user_db.user_count++;

    printk("[USER] Created user '%s' (uid=%d, gid=%d)\n", username, uid, gid);
    return 0;
}

/* Delete user account */
int user_delete(const char *username) {
    if (!username) {
        return -1;
    }

    /* Find user */
    for (int i = 0; i < MAX_USERS; i++) {
        if (user_db.accounts[i].in_use &&
            strcmp(user_db.accounts[i].username, username) == 0) {

            /* Don't allow deleting root */
            if (user_db.accounts[i].uid == 0) {
                return -1;
            }

            /* Clear account and password */
            memset(&user_db.accounts[i], 0, sizeof(user_account_t));
            memset(&user_db.passwords[i], 0, sizeof(password_entry_t));
            user_db.user_count--;

            printk("[USER] Deleted user '%s'\n", username);
            return 0;
        }
    }

    return -1;  /* User not found */
}

/* Get user account by username */
user_account_t *user_get_by_name(const char *username) {
    if (!username) {
        return NULL;
    }

    for (int i = 0; i < MAX_USERS; i++) {
        if (user_db.accounts[i].in_use &&
            strcmp(user_db.accounts[i].username, username) == 0) {
            return &user_db.accounts[i];
        }
    }

    return NULL;
}

/* Get user account by UID */
user_account_t *user_get_by_uid(uint32_t uid) {
    for (int i = 0; i < MAX_USERS; i++) {
        if (user_db.accounts[i].in_use && user_db.accounts[i].uid == uid) {
            return &user_db.accounts[i];
        }
    }

    return NULL;
}

/* Set user password */
int user_set_password(const char *username, const char *password) {
    if (!username || !password) {
        return -1;
    }

    /* Find user */
    for (int i = 0; i < MAX_USERS; i++) {
        if (user_db.passwords[i].in_use &&
            strcmp(user_db.passwords[i].username, username) == 0) {

            /* Hash and store new password */
            user_hash_password(password, user_db.passwords[i].password_hash,
                             USER_PASSWORD_HASH_MAX);

            printk("[USER] Password updated for user '%s'\n", username);
            return 0;
        }
    }

    return -1;  /* User not found */
}

/* Verify user password */
int user_verify_password(const char *username, const char *password) {
    if (!username || !password) {
        return 0;
    }

    /* Find password entry */
    for (int i = 0; i < MAX_USERS; i++) {
        if (user_db.passwords[i].in_use &&
            strcmp(user_db.passwords[i].username, username) == 0) {

            /* Hash provided password and compare */
            char hash[USER_PASSWORD_HASH_MAX];
            user_hash_password(password, hash, sizeof(hash));

            return (strcmp(hash, user_db.passwords[i].password_hash) == 0);
        }
    }

    return 0;  /* User not found */
}

/* Login user */
int user_login(const char *username, const char *password) {
    if (!username || !password) {
        return -1;
    }

    /* Verify password */
    if (!user_verify_password(username, password)) {
        printk("[USER] Login failed for user '%s': Invalid password\n", username);
        return -1;
    }

    /* Get user account */
    user_account_t *account = user_get_by_name(username);
    if (!account) {
        return -1;
    }

    /* Set current user */
    current_uid = account->uid;
    strncpy(current_username, username, USER_NAME_MAX - 1);
    current_username[USER_NAME_MAX - 1] = '\0';

    printk("[USER] User '%s' logged in successfully (uid=%d)\n", username, account->uid);
    return 0;
}

/* Get current UID */
uint32_t user_get_current_uid(void) {
    return current_uid;
}

/* Get current username */
const char *user_get_current_username(void) {
    return current_username;
}

/* Print user accounts */
void user_print_accounts(void) {
    printk("=== User Accounts (/etc/passwd) ===\n");
    for (int i = 0; i < MAX_USERS; i++) {
        if (user_db.accounts[i].in_use) {
            printk("%s:x:%d:%d:%s:%s\n",
                   user_db.accounts[i].username,
                   user_db.accounts[i].uid,
                   user_db.accounts[i].gid,
                   user_db.accounts[i].home,
                   user_db.accounts[i].shell);
        }
    }
}

/* ===== System Calls ===== */

/* sys_setuid - Set user ID (only root can do this) */
int sys_setuid(uint32_t uid, uint32_t unused1, uint32_t unused2, uint32_t unused3, uint32_t unused4) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4;

    /* Only root can change UID */
    if (current_uid != 0) {
        return -1;  /* Permission denied */
    }

    /* Check if UID exists */
    user_account_t *account = user_get_by_uid(uid);
    if (!account) {
        return -1;  /* UID not found */
    }

    /* Change UID */
    current_uid = uid;
    strncpy(current_username, account->username, USER_NAME_MAX - 1);
    current_username[USER_NAME_MAX - 1] = '\0';

    printk("[USER] UID changed to %d (%s)\n", uid, account->username);
    return 0;
}
