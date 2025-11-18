/* env.c - Unix environment variables implementation */

#include "../include/env.h"
#include "../include/printf.h"
#include "../include/string.h"
#include "../include/kmalloc.h"
#include "../include/process.h"

/* Initialize environment system */
void env_init(void) {
    printk("[ENV] Environment system initialized\n");
}

/* Create a new environment table with default values */
env_table_t *env_create_default(void) {
    env_table_t *env = (env_table_t *)kmalloc(sizeof(env_table_t));
    if (!env) {
        return NULL;
    }

    memset(env, 0, sizeof(env_table_t));
    env->count = 0;

    /* Set default environment variables */
    env_set(env, "PATH", "/bin:/usr/bin");
    env_set(env, "HOME", "/root");
    env_set(env, "SHELL", "/bin/sh");
    env_set(env, "USER", "root");
    env_set(env, "LOGNAME", "root");
    env_set(env, "TERM", "console");
    env_set(env, "PWD", "/root");

    return env;
}

/* Create a copy of an environment table (for fork) */
env_table_t *env_copy(env_table_t *src) {
    if (!src) {
        return env_create_default();
    }

    env_table_t *dst = (env_table_t *)kmalloc(sizeof(env_table_t));
    if (!dst) {
        return NULL;
    }

    memcpy(dst, src, sizeof(env_table_t));
    return dst;
}

/* Destroy environment table */
void env_destroy(env_table_t *env) {
    if (env) {
        kfree(env);
    }
}

/* Find environment variable by name */
static env_var_t *env_find(env_table_t *env, const char *name) {
    if (!env || !name) {
        return NULL;
    }

    for (int i = 0; i < MAX_ENV_VARS; i++) {
        if (env->vars[i].in_use && strcmp(env->vars[i].name, name) == 0) {
            return &env->vars[i];
        }
    }

    return NULL;
}

/* Find free slot in environment table */
static env_var_t *env_find_free(env_table_t *env) {
    if (!env) {
        return NULL;
    }

    for (int i = 0; i < MAX_ENV_VARS; i++) {
        if (!env->vars[i].in_use) {
            return &env->vars[i];
        }
    }

    return NULL;
}

/* Get environment variable value */
char *env_get(env_table_t *env, const char *name) {
    env_var_t *var = env_find(env, name);
    if (var) {
        return var->value;
    }
    return NULL;
}

/* Set environment variable */
int env_set(env_table_t *env, const char *name, const char *value) {
    if (!env || !name || !value) {
        return -1;
    }

    /* Check name and value length */
    if (strlen(name) >= ENV_NAME_MAX || strlen(value) >= ENV_VALUE_MAX) {
        return -1;
    }

    /* Check if variable already exists */
    env_var_t *var = env_find(env, name);
    if (var) {
        /* Update existing variable */
        strncpy(var->value, value, ENV_VALUE_MAX - 1);
        var->value[ENV_VALUE_MAX - 1] = '\0';
        return 0;
    }

    /* Find free slot */
    var = env_find_free(env);
    if (!var) {
        return -1;  /* No free slots */
    }

    /* Create new variable */
    strncpy(var->name, name, ENV_NAME_MAX - 1);
    var->name[ENV_NAME_MAX - 1] = '\0';
    strncpy(var->value, value, ENV_VALUE_MAX - 1);
    var->value[ENV_VALUE_MAX - 1] = '\0';
    var->in_use = 1;
    env->count++;

    return 0;
}

/* Unset environment variable */
int env_unset(env_table_t *env, const char *name) {
    env_var_t *var = env_find(env, name);
    if (!var) {
        return -1;  /* Variable not found */
    }

    /* Clear variable */
    memset(var, 0, sizeof(env_var_t));
    var->in_use = 0;
    env->count--;

    return 0;
}

/* Print all environment variables */
void env_print(env_table_t *env) {
    if (!env) {
        printk("(null environment)\n");
        return;
    }

    printk("Environment (%d variables):\n", env->count);
    for (int i = 0; i < MAX_ENV_VARS; i++) {
        if (env->vars[i].in_use) {
            printk("  %s=%s\n", env->vars[i].name, env->vars[i].value);
        }
    }
}

/* ===== System Calls ===== */

/* sys_getenv - Get environment variable */
int sys_getenv(uint32_t name_ptr, uint32_t buf_ptr, uint32_t buf_size, uint32_t unused1, uint32_t unused2) {
    (void)unused1; (void)unused2;

    process_t *proc = process_get_current();
    if (!proc || !proc->environment) {
        return -1;
    }

    char *name = (char *)name_ptr;
    char *buf = (char *)buf_ptr;

    if (!name || !buf || buf_size == 0) {
        return -1;
    }

    char *value = env_get(proc->environment, name);
    if (!value) {
        return -1;  /* Variable not found */
    }

    /* Copy value to user buffer */
    size_t len = strlen(value);
    if (len >= buf_size) {
        len = buf_size - 1;
    }

    strncpy(buf, value, len);
    buf[len] = '\0';

    return (int)len;
}

/* sys_setenv - Set environment variable */
int sys_setenv(uint32_t name_ptr, uint32_t value_ptr, uint32_t overwrite, uint32_t unused1, uint32_t unused2) {
    (void)unused1; (void)unused2;

    process_t *proc = process_get_current();
    if (!proc || !proc->environment) {
        return -1;
    }

    char *name = (char *)name_ptr;
    char *value = (char *)value_ptr;

    if (!name || !value) {
        return -1;
    }

    /* Check if variable exists and overwrite is false */
    if (!overwrite && env_get(proc->environment, name)) {
        return 0;  /* Don't overwrite */
    }

    return env_set(proc->environment, name, value);
}

/* sys_unsetenv - Unset environment variable */
int sys_unsetenv(uint32_t name_ptr, uint32_t unused1, uint32_t unused2, uint32_t unused3, uint32_t unused4) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4;

    process_t *proc = process_get_current();
    if (!proc || !proc->environment) {
        return -1;
    }

    char *name = (char *)name_ptr;
    if (!name) {
        return -1;
    }

    return env_unset(proc->environment, name);
}
