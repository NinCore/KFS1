/* timer.h - Programmable Interval Timer (PIT) for preemptive multitasking */

#ifndef TIMER_H
#define TIMER_H

#include "types.h"

/* PIT frequency (in Hz) - oscillates at 1.193182 MHz */
#define PIT_FREQUENCY 1193182

/* Desired timer frequency (in Hz) - how often to trigger scheduler */
#define TIMER_FREQUENCY 100  /* 100 Hz = 10ms per tick */

/* Timer ticks since boot */
extern volatile uint32_t timer_ticks;

/* Initialize the PIT timer */
void timer_init(uint32_t frequency);

/* Get current timer ticks */
uint32_t timer_get_ticks(void);

/* Wait for specified number of ticks */
void timer_wait(uint32_t ticks);

#endif /* TIMER_H */
