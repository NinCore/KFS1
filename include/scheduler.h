/* scheduler.h - Process scheduler interface */

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "types.h"
#include "idt.h"

/* Forward declaration */
struct process;

/* Initialize scheduler */
void scheduler_init(void);

/* Add a process to the ready queue */
void scheduler_add(struct process *proc);

/* Remove a process from the ready queue */
void scheduler_remove(struct process *proc);

/* Run the next process in the ready queue */
void scheduler_run(void);

/* Start the scheduler (runs until all processes complete) */
void scheduler_start(void);

/* Timer interrupt handler for preemptive scheduling */
void scheduler_timer_tick(struct interrupt_frame *frame);

#endif /* SCHEDULER_H */
