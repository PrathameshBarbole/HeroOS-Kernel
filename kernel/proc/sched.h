#ifndef KERNEL_SCHED_H
#define KERNEL_SCHED_H

#include <kernel/types.h>
#include <kernel/proc/process.h>

/* Scheduler policy */
typedef enum {
    SCHED_FIFO     = 0,  /* First-in, first-out (real-time) */
    SCHED_RR       = 1,  /* Round-robin */
    SCHED_CFS      = 2,  /* Completely Fair Scheduler (default) */
    SCHED_IDLE     = 3,  /* Runs only when nothing else can */
} sched_policy_t;

/* Scheduler time slice (in PIT ticks) */
#define SCHED_DEFAULT_QUANTUM   10   /* 10ms at 1000 Hz */
#define SCHED_MIN_QUANTUM       2
#define SCHED_MAX_QUANTUM       100

/* Run queue (per-CPU, single for now) */
typedef struct {
    process_t  *head;         /* Front of ready queue */
    process_t  *tail;
    uint32_t    count;        /* Number of runnable processes */
    process_t  *current;      /* Currently executing process */
    uint64_t    tick;         /* Scheduler tick counter */
} run_queue_t;

/* Scheduler API */
void     sched_init(void);
void     sched_add(process_t *proc);
void     sched_remove(process_t *proc);
void     sched_tick(void);           /* Called from PIT ISR */
void     sched_yield(void);          /* Voluntarily give up CPU */
void     sched_sleep(uint64_t ms);   /* Sleep current process */
void     sched_wake(process_t *proc);
process_t *sched_current(void);

extern run_queue_t g_runqueue;

#endif /* KERNEL_SCHED_H */
