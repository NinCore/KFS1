/* signal.h - Signal handling system */

#ifndef SIGNAL_H
#define SIGNAL_H

#include "types.h"
#include "idt.h"

/* Signal numbers (similar to POSIX) */
#define SIG_DFL     ((signal_handler_t)0)  /* Default action */
#define SIG_IGN     ((signal_handler_t)1)  /* Ignore signal */

#define SIGHUP      1   /* Hangup */
#define SIGINT      2   /* Interrupt */
#define SIGQUIT     3   /* Quit */
#define SIGILL      4   /* Illegal instruction */
#define SIGTRAP     5   /* Trace trap */
#define SIGABRT     6   /* Abort */
#define SIGBUS      7   /* Bus error */
#define SIGFPE      8   /* Floating point exception */
#define SIGKILL     9   /* Kill (cannot be caught) */
#define SIGUSR1     10  /* User-defined signal 1 */
#define SIGSEGV     11  /* Segmentation violation */
#define SIGUSR2     12  /* User-defined signal 2 */
#define SIGPIPE     13  /* Broken pipe */
#define SIGALRM     14  /* Alarm clock */
#define SIGTERM     15  /* Termination */
#define SIGSTKFLT   16  /* Stack fault */
#define SIGCHLD     17  /* Child stopped */
#define SIGCONT     18  /* Continue */
#define SIGSTOP     19  /* Stop */
#define SIGTSTP     20  /* Keyboard stop */

#define MAX_SIGNALS 32

/* Signal action structure */
struct signal_action {
    signal_handler_t handler;
    bool registered;
};

/* Initialize signal system */
void signal_init(void);

/* Register a signal handler */
int signal_register(int signal, signal_handler_t handler);

/* Unregister a signal handler */
int signal_unregister(int signal);

/* Raise a signal (schedule it for execution) */
int signal_raise(int signal);

/* Send a signal immediately (synchronous) */
int signal_send(int signal);

/* Process pending signals (called from scheduler) */
void signal_process_pending(void);

/* Get signal name for debugging */
const char* signal_name(int signal);

#endif /* SIGNAL_H */
