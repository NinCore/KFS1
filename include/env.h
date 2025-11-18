/* env.h - Unix environment variables system for KFS-7 */

#ifndef ENV_H
#define ENV_H

#include "types.h"

/* Maximum environment variables per process */
#define MAX_ENV_VARS 64

/* Maximum length of environment variable name and value */
#define ENV_NAME_MAX 64
#define ENV_VALUE_MAX 256

/* Environment variable entry */
typedef struct env_var {
    char name[ENV_NAME_MAX];
    char value[ENV_VALUE_MAX];
    int in_use;
} env_var_t;

/* Environment variables table for a process */
typedef struct env_table {
    env_var_t vars[MAX_ENV_VARS];
    int count;
} env_table_t;

/* Initialize environment system */
void env_init(void);

/* Create a new environment table with defaults */
env_table_t *env_create_default(void);

/* Create a copy of an environment table (for fork) */
env_table_t *env_copy(env_table_t *src);

/* Destroy environment table */
void env_destroy(env_table_t *env);

/* Get environment variable value */
char *env_get(env_table_t *env, const char *name);

/* Set environment variable */
int env_set(env_table_t *env, const char *name, const char *value);

/* Unset environment variable */
int env_unset(env_table_t *env, const char *name);

/* Print all environment variables (for debugging) */
void env_print(env_table_t *env);

/* Get environment as array of strings (for execve) */
char **env_to_array(env_table_t *env);

/* System calls */
int sys_getenv(uint32_t name_ptr, uint32_t buf_ptr, uint32_t buf_size, uint32_t unused1, uint32_t unused2);
int sys_setenv(uint32_t name_ptr, uint32_t value_ptr, uint32_t overwrite, uint32_t unused1, uint32_t unused2);
int sys_unsetenv(uint32_t name_ptr, uint32_t unused1, uint32_t unused2, uint32_t unused3, uint32_t unused4);

#endif /* ENV_H */
