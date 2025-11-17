/* shell.h - Minimalistic kernel shell for debugging */

#ifndef SHELL_H
#define SHELL_H

#include "types.h"

#define SHELL_BUFFER_SIZE 256
#define SHELL_MAX_ARGS 16

/* Initialize the shell */
void shell_init(void);

/* Run the shell main loop */
void shell_run(void);

/* Process a command */
void shell_process_command(const char *command);

/* Handle keyboard input for shell */
void shell_handle_input(int c);

#endif /* SHELL_H */
