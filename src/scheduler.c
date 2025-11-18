/* scheduler.c - Process scheduler implementation */

#include "../include/process.h"
#include "../include/printf.h"
#include "../include/idt.h"
#include "../include/pic.h"
#include "../include/io.h"

/* Forward declaration */
void scheduler_timer_tick(struct interrupt_frame *frame);

/* Scheduler ready queue */
#define READY_QUEUE_SIZE 256
static struct process *ready_queue[READY_QUEUE_SIZE];
static int queue_head = 0;
static int queue_tail = 0;
static int queue_count = 0;

/* Timer frequency for scheduling (Hz) */
#define SCHEDULER_FREQUENCY 100  /* 100 Hz = 10ms time slices */

/* PIT (Programmable Interval Timer) ports */
#define PIT_CHANNEL0 0x40
#define PIT_COMMAND  0x43

/* Initialize scheduler */
void scheduler_init(void) {
    queue_head = 0;
    queue_tail = 0;
    queue_count = 0;

    /* Set up PIT for scheduler timer */
    uint32_t divisor = 1193180 / SCHEDULER_FREQUENCY;

    /* Command: Channel 0, lo/hi byte, rate generator */
    outb(PIT_COMMAND, 0x36);

    /* Send divisor */
    outb(PIT_CHANNEL0, divisor & 0xFF);
    outb(PIT_CHANNEL0, (divisor >> 8) & 0xFF);

    /* Register timer interrupt handler (IRQ0) */
    idt_register_handler(IRQ0, (interrupt_handler_t)scheduler_timer_tick);

    printk("[SCHEDULER] Initialized with %d Hz timer\n", SCHEDULER_FREQUENCY);
}

/* Add process to ready queue */
void scheduler_add(struct process *proc) {
    if (!proc || queue_count >= READY_QUEUE_SIZE) {
        return;
    }

    proc->state = PROCESS_STATE_READY;
    ready_queue[queue_tail] = proc;
    queue_tail = (queue_tail + 1) % READY_QUEUE_SIZE;
    queue_count++;
}

/* Remove process from ready queue */
void scheduler_remove(struct process *proc) {
    if (!proc || queue_count == 0) {
        return;
    }

    /* Find and remove process */
    for (int i = 0; i < READY_QUEUE_SIZE; i++) {
        int idx = (queue_head + i) % READY_QUEUE_SIZE;
        if (ready_queue[idx] == proc) {
            /* Shift elements */
            for (int j = i; j < queue_count - 1; j++) {
                int cur = (queue_head + j) % READY_QUEUE_SIZE;
                int next = (queue_head + j + 1) % READY_QUEUE_SIZE;
                ready_queue[cur] = ready_queue[next];
            }
            queue_count--;
            queue_tail = (queue_tail - 1 + READY_QUEUE_SIZE) % READY_QUEUE_SIZE;
            break;
        }
    }
}

/* Get next process to run (round-robin) */
struct process *scheduler_next(void) {
    if (queue_count == 0) {
        return NULL;
    }

    struct process *next = ready_queue[queue_head];
    queue_head = (queue_head + 1) % READY_QUEUE_SIZE;
    queue_count--;

    return next;
}

/* Schedule next process */
void scheduler_run(void) {
    struct process *next = scheduler_next();

    if (!next) {
        /* No processes to run, return to kernel */
        current_process = NULL;
        return;
    }

    struct process *prev = current_process;

    /* If previous process is still runnable, add it back to queue */
    if (prev && prev->state == PROCESS_STATE_RUNNING) {
        prev->state = PROCESS_STATE_READY;
        scheduler_add(prev);
    }

    /* Switch to next process */
    next->state = PROCESS_STATE_RUNNING;
    current_process = next;

    /* Deliver any pending signals */
    process_deliver_signals(next);

    /* Perform context switch */
    if (prev) {
        context_switch(prev, next);
    }
}

/* Timer tick handler for preemptive multitasking */
void scheduler_timer_tick(struct interrupt_frame *frame) {
    (void)frame;

    /* Increment time for current process */
    if (current_process) {
        current_process->total_time++;
    }

    /* Preempt and schedule next process */
    scheduler_run();

    /* Send EOI to PIC */
    pic_send_eoi(0);
}

/* Start scheduling - called from kernel main */
void scheduler_start(void) {
    printk("[SCHEDULER] Starting multitasking\n");

    /* Enable timer interrupt */
    interrupts_enable();

    /* Run first process */
    scheduler_run();
}
