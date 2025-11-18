/* signal.c - Signal handling system implementation */

#include "../include/signal.h"
#include "../include/printf.h"
#include "../include/string.h"

/* Signal action table */
static struct signal_action signal_actions[MAX_SIGNALS];

/* Pending signals queue */
static uint32_t pending_signals = 0;

/* Signal names for debugging */
static const char *signal_names[] = {
    "SIGNAL0",   /* 0 - unused */
    "SIGHUP",    /* 1 */
    "SIGINT",    /* 2 */
    "SIGQUIT",   /* 3 */
    "SIGILL",    /* 4 */
    "SIGTRAP",   /* 5 */
    "SIGABRT",   /* 6 */
    "SIGBUS",    /* 7 */
    "SIGFPE",    /* 8 */
    "SIGKILL",   /* 9 */
    "SIGUSR1",   /* 10 */
    "SIGSEGV",   /* 11 */
    "SIGUSR2",   /* 12 */
    "SIGPIPE",   /* 13 */
    "SIGALRM",   /* 14 */
    "SIGTERM",   /* 15 */
    "SIGSTKFLT", /* 16 */
    "SIGCHLD",   /* 17 */
    "SIGCONT",   /* 18 */
    "SIGSTOP",   /* 19 */
    "SIGTSTP",   /* 20 */
};

/* Initialize signal system */
void signal_init(void) {
    /* Clear all signal actions */
    for (int i = 0; i < MAX_SIGNALS; i++) {
        signal_actions[i].handler = SIG_DFL;
        signal_actions[i].registered = false;
    }
    pending_signals = 0;
}

/* Validate signal number */
static bool signal_valid(int signal) {
    return (signal > 0 && signal < MAX_SIGNALS);
}

/* Register a signal handler */
int signal_register(int signal, signal_handler_t handler) {
    if (!signal_valid(signal)) {
        return -1;  /* Invalid signal */
    }

    /* SIGKILL and SIGSTOP cannot be caught */
    if (signal == SIGKILL || signal == SIGSTOP) {
        return -1;
    }

    signal_actions[signal].handler = handler;
    signal_actions[signal].registered = true;

    return 0;
}

/* Unregister a signal handler */
int signal_unregister(int signal) {
    if (!signal_valid(signal)) {
        return -1;
    }

    signal_actions[signal].handler = SIG_DFL;
    signal_actions[signal].registered = false;

    return 0;
}

/* Raise a signal (schedule it for later execution) */
int signal_raise(int signal) {
    if (!signal_valid(signal)) {
        return -1;
    }

    /* Mark signal as pending */
    pending_signals |= (1 << signal);

    return 0;
}

/* Send a signal immediately (synchronous) */
int signal_send(int signal) {
    if (!signal_valid(signal)) {
        return -1;
    }

    signal_handler_t handler = signal_actions[signal].handler;

    /* Check if handler is registered and not default/ignore */
    if (signal_actions[signal].registered &&
        handler != SIG_DFL &&
        handler != SIG_IGN) {
        /* Call the signal handler */
        handler(signal);
    } else if (handler == SIG_DFL) {
        /* Default action - for now, just print a message */
        printk("[SIGNAL] %s (default action)\n", signal_name(signal));
    }
    /* SIG_IGN - do nothing */

    return 0;
}

/* Process pending signals */
void signal_process_pending(void) {
    if (pending_signals == 0) {
        return;
    }

    /* Process each pending signal */
    for (int i = 1; i < MAX_SIGNALS; i++) {
        if (pending_signals & (1 << i)) {
            /* Clear pending flag */
            pending_signals &= ~(1 << i);

            /* Send the signal */
            signal_send(i);
        }
    }
}

/* Get signal name */
const char* signal_name(int signal) {
    if (signal < 0 || signal > 20) {
        return "UNKNOWN";
    }
    return signal_names[signal];
}
