#include <kernel/proc/sched.h>
#include <kernel/printk.h>
#include <lib/string.h>

/* ─── Global run queue ───────────────────────────────────────────────────── */

run_queue_t g_runqueue;

/* ─── Init ───────────────────────────────────────────────────────────────── */

void sched_init(void) {
    memset(&g_runqueue, 0, sizeof(run_queue_t));

    /* Start on the idle process (PID 0) */
    extern process_t *proc_get(pid_t pid);
    g_runqueue.current = proc_get(0);

    pr_info("Scheduler initialised (CFS-inspired round-robin)\n");
}

/* ─── Queue management ───────────────────────────────────────────────────── */

void sched_add(process_t *proc) {
    if (!proc || proc->state != PROC_READY) return;

    /* Append to tail */
    proc->next = NULL;
    proc->prev = g_runqueue.tail;
    if (g_runqueue.tail)
        g_runqueue.tail->next = proc;
    else
        g_runqueue.head = proc;
    g_runqueue.tail = proc;
    g_runqueue.count++;
}

void sched_remove(process_t *proc) {
    if (!proc) return;
    if (proc->prev) proc->prev->next = proc->next;
    else            g_runqueue.head  = proc->next;
    if (proc->next) proc->next->prev = proc->prev;
    else            g_runqueue.tail  = proc->prev;
    proc->next = proc->prev = NULL;
    if (g_runqueue.count > 0) g_runqueue.count--;
}

/* ─── Pick next process (CFS-inspired: lowest vruntime) ─────────────────── */

static process_t *pick_next(void) {
    if (!g_runqueue.head) {
        /* Fall back to idle */
        extern process_t *proc_get(pid_t pid);
        return proc_get(0);
    }

    process_t *best = NULL;
    process_t *cur  = g_runqueue.head;
    while (cur) {
        if (cur->state == PROC_READY) {
            if (!best || cur->vruntime < best->vruntime)
                best = cur;
        }
        cur = cur->next;
    }
    return best ? best : proc_get(0);
}

/* ─── Scheduler tick (called from PIT IRQ handler) ──────────────────────── */

void sched_tick(void) {
    g_runqueue.tick++;
    process_t *current = g_runqueue.current;

    if (current) {
        current->runtime_ticks++;
        /* Increase vruntime proportionally to inverse of priority */
        current->vruntime += (255 - current->priority + 1);

        /* Wake sleeping processes */
        process_t *p = g_runqueue.head;
        while (p) {
            if (p->state == PROC_SLEEPING &&
                g_runqueue.tick >= p->sleep_until) {
                p->state = PROC_READY;
            }
            p = p->next;
        }

        /* Time slice expired — preempt */
        if (current->runtime_ticks % SCHED_DEFAULT_QUANTUM == 0) {
            if (current->state == PROC_RUNNING) {
                current->state = PROC_READY;
            }
            process_t *next = pick_next();
            if (next && next != current) {
                g_runqueue.current  = next;
                next->state         = PROC_RUNNING;
                if (current->state  == PROC_READY)
                    current->state  = PROC_READY;   /* Stays runnable */
                context_switch(&current->context, &next->context);
            }
        }
    }
}

/* ─── Voluntary yield ────────────────────────────────────────────────────── */

void sched_yield(void) {
    process_t *current = g_runqueue.current;
    if (!current) return;

    current->state = PROC_READY;
    process_t *next = pick_next();
    if (next && next != current) {
        g_runqueue.current = next;
        next->state        = PROC_RUNNING;
        context_switch(&current->context, &next->context);
    }
}

/* ─── Sleep ──────────────────────────────────────────────────────────────── */

void sched_sleep(uint64_t ms) {
    process_t *current = g_runqueue.current;
    if (!current) return;

    current->state       = PROC_SLEEPING;
    current->sleep_until = g_runqueue.tick + ms;   /* tick rate = 1 kHz */
    sched_yield();
}

/* ─── Wake ───────────────────────────────────────────────────────────────── */

void sched_wake(process_t *proc) {
    if (!proc) return;
    if (proc->state == PROC_SLEEPING || proc->state == PROC_BLOCKED) {
        proc->state = PROC_READY;
    }
}

process_t *sched_current(void) {
    return g_runqueue.current;
}
