/* process_test.c - Process testing and demonstration */

#include "../include/process.h"
#include "../include/printf.h"
#include "../include/vga.h"
#include "../include/string.h"
#include "../include/syscall.h"
#include "../include/scheduler.h"

/* Test process 1 - Counts and prints */
void test_process_1(void) {
    for (int i = 0; i < 10; i++) {
        vga_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        printk("[PROCESS 1] Count: %d\n", i);
        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

        /* Busy wait to simulate work */
        for (volatile int j = 0; j < 1000000; j++);
    }

    printk("[PROCESS 1] Exiting\n");
    process_exit(0);
}

/* Test process 2 - Prints letters */
void test_process_2(void) {
    const char *msg = "ABCDEFGHIJ";
    for (int i = 0; msg[i]; i++) {
        vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
        printk("[PROCESS 2] Letter: %c\n", msg[i]);
        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

        /* Busy wait to simulate work */
        for (volatile int j = 0; j < 1000000; j++);
    }

    printk("[PROCESS 2] Exiting\n");
    process_exit(0);
}

/* Test process 3 - Prints symbols */
void test_process_3(void) {
    const char *symbols = "!@#$%^&*()";
    for (int i = 0; symbols[i]; i++) {
        vga_set_color(VGA_COLOR_LIGHT_MAGENTA, VGA_COLOR_BLACK);
        printk("[PROCESS 3] Symbol: %c\n", symbols[i]);
        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

        /* Busy wait to simulate work */
        for (volatile int j = 0; j < 1000000; j++);
    }

    printk("[PROCESS 3] Exiting\n");
    process_exit(0);
}

/* Test fork() */
void test_fork_process(void) {
    printk("[FORK TEST] Parent process starting\n");

    int pid = process_fork();

    if (pid == 0) {
        /* Child process */
        for (int i = 0; i < 5; i++) {
            vga_set_color(VGA_COLOR_LIGHT_BLUE, VGA_COLOR_BLACK);
            printk("  [CHILD] Iteration %d\n", i);
            vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
            for (volatile int j = 0; j < 1000000; j++);
        }
        printk("  [CHILD] Exiting\n");
        process_exit(42);
    } else {
        /* Parent process */
        for (int i = 0; i < 5; i++) {
            vga_set_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK);
            printk("[PARENT] Child PID: %d, Iteration %d\n", pid, i);
            vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
            for (volatile int j = 0; j < 1000000; j++);
        }

        /* Wait for child */
        int status;
        int child_pid = process_wait(&status);
        printk("[PARENT] Child %d exited with status %d\n", child_pid, status);
        process_exit(0);
    }
}

/* Signal handler test */
void test_signal_handler(int sig) {
    vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    printk("[SIGNAL] Received signal %d (%s) in process %d\n",
           sig, signal_name(sig), current_process ? current_process->pid : 0);
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
}

/* Test signals between processes */
void test_signal_process(void) {
    printk("[SIGNAL TEST] Installing signal handler\n");

    /* Install signal handler for SIGUSR1 */
    sys_signal(SIGUSR1, (uint32_t)test_signal_handler, 0, 0, 0);

    printk("[SIGNAL TEST] Waiting for signal...\n");

    /* Wait a bit */
    for (int i = 0; i < 10; i++) {
        printk("[SIGNAL TEST] Waiting... %d\n", i);
        for (volatile int j = 0; j < 2000000; j++);

        /* Process pending signals */
        if (current_process) {
            process_deliver_signals(current_process);
        }
    }

    printk("[SIGNAL TEST] Exiting\n");
    process_exit(0);
}

/* Main test function */
void process_test_suite(void) {
    vga_clear();
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE);
    printk("===========================================\n");
    printk("   KFS_5 - Process Multitasking Test      \n");
    printk("===========================================\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    printk("\n");

    /* Initialize process subsystem */
    process_init();
    scheduler_init();

    printk("[TEST] Creating test processes...\n\n");

    /* Create test processes using exec_fn */
    exec_fn((uint32_t)test_process_1, test_process_1, 0);
    exec_fn((uint32_t)test_process_2, test_process_2, 0);
    exec_fn((uint32_t)test_process_3, test_process_3, 0);

    printk("[TEST] Starting scheduler...\n\n");

    /* Start multitasking */
    scheduler_start();

    printk("\n[TEST] All processes completed!\n");
}

/* Test fork */
void process_test_fork(void) {
    vga_clear();
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE);
    printk("===========================================\n");
    printk("      KFS_5 - Fork Test                   \n");
    printk("===========================================\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    printk("\n");

    process_init();
    scheduler_init();

    exec_fn((uint32_t)test_fork_process, test_fork_process, 0);

    scheduler_start();

    printk("\n[TEST] Fork test completed!\n");
}

/* Test signals */
void process_test_signals(void) {
    vga_clear();
    vga_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE);
    printk("===========================================\n");
    printk("      KFS_5 - Signal Test                 \n");
    printk("===========================================\n");
    vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    printk("\n");

    process_init();
    scheduler_init();

    /* Create signal test process */
    struct process *proc = process_create(test_signal_process, 0);
    if (proc) {
        scheduler_add(proc);

        /* Send signal after a delay */
        printk("[TEST] Will send SIGUSR1 to process %d\n\n", proc->pid);

        /* Start scheduler */
        scheduler_start();

        /* Send signal (this would normally be from another process/interrupt) */
        for (volatile int i = 0; i < 5000000; i++);
        process_kill(proc->pid, SIGUSR1);
    }

    printk("\n[TEST] Signal test completed!\n");
}
