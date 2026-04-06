#include "pulse.h"
#include <kernel/printk.h>
#include <kernel/proc/process.h>
#include <kernel/proc/sched.h>
#include <kernel/mm/pmm.h>
#include <lib/string.h>

/* ── Collect a system snapshot ───────────────────────────────────────────── */

void pulse_collect(pulse_snapshot_t *snap) {
    if (!snap) return;
    memset(snap, 0, sizeof(*snap));

    /* Memory */
    pmm_stats_t pmm;
    pmm_get_stats(&pmm);
    snap->memory.total  = pmm.total_pages * PAGE_SIZE;
    snap->memory.used   = pmm.used_pages  * PAGE_SIZE;
    snap->memory.free   = pmm.free_pages  * PAGE_SIZE;

    /* Uptime via PIT ticks (1 kHz = 1 tick per ms) */
    extern uint64_t pit_get_ticks(void);
    snap->uptime_sec = pit_get_ticks() / 1000;

    /* TODO: populate cpu, disk, nic, processes from kernel subsystems */
    snap->cpu.core_count = 1;
    strncpy(snap->cpu.model, "x86_64", 127);
}

/* ── Formatted output ────────────────────────────────────────────────────── */

void pulse_print_overview(const pulse_snapshot_t *snap) {
    uint64_t uptime_m = snap->uptime_sec / 60;
    uint64_t uptime_s = snap->uptime_sec % 60;

    printk("\n");
    printk("┌──────────────────── Pulse ─────────────────────────┐\n");
    printk("│ Uptime  : %llu min %llu sec                         \n",
           uptime_m, uptime_s);
    printk("│ CPU     : %.1f%%                                    \n",
           (double)snap->cpu.total_usage);
    printk("│ Memory  : %llu / %llu MiB (%.0f%%)                 \n",
           snap->memory.used  >> 20,
           snap->memory.total >> 20,
           snap->memory.total
               ? (double)snap->memory.used * 100.0 / (double)snap->memory.total
               : 0.0);
    printk("│ Processes: %u                                       \n",
           snap->process_count);
    printk("└────────────────────────────────────────────────────┘\n");
}

void pulse_print_processes(const pulse_snapshot_t *snap) {
    printk("PID   CPU%%  MEM(MB)  NAME\n");
    printk("----- ----- -------- ----------------\n");
    for (uint32_t i = 0; i < snap->process_count && i < 20; i++) {
        const pulse_process_t *p = &snap->processes[i];
        printk("%-5u %-5.1f %-8llu %s\n",
               p->pid,
               (double)p->cpu_percent,
               p->mem_bytes >> 20,
               p->name);
    }
}

/* ── Alert thresholds (printk-based for now) ─────────────────────────────── */

static float alert_cpu = 90.0f;
static float alert_mem = 90.0f;

void pulse_set_alert_cpu(float t) { alert_cpu = t; }
void pulse_set_alert_mem(float t) { alert_mem = t; }

/* ── Init / main ──────────────────────────────────────────────────────────── */

void pulse_init(void) {
    pr_info("Pulse: system monitor initialised\n");
}

void pulse_main(void) {
    pr_info("Pulse: starting system monitor\n");

    pulse_snapshot_t snap;
    for (;;) {
        pulse_collect(&snap);

        /* Alert checks */
        if (snap.cpu.total_usage > alert_cpu)
            pr_warn("Pulse: CPU usage %.1f%% exceeds threshold\n",
                    (double)snap.cpu.total_usage);
        if (snap.memory.total > 0) {
            float mem_pct = (float)snap.memory.used * 100.0f / (float)snap.memory.total;
            if (mem_pct > alert_mem)
                pr_warn("Pulse: memory usage %.0f%% exceeds threshold\n",
                        (double)mem_pct);
        }

        sched_sleep(1000);   /* Sample once per second */
    }
}
