/*
 * Pulse — HeroOS System Monitor & Developer Dashboard
 *
 * Real-time system telemetry with a premium, developer-friendly UI.
 *
 * Panels:
 *   Overview  — CPU, RAM, GPU, network at a glance (live graphs)
 *   Processes — sortable process table (like htop, but prettier)
 *   CPU       — per-core usage, frequency, temperature
 *   Memory    — RAM/swap breakdown, top consumers, heap stats
 *   Disk      — I/O per device, read/write speeds, SMART status
 *   Network   — packets/sec, bandwidth per process
 *   GPU       — framebuffer usage, render time
 *   Battery   — charge level, drain rate, estimated time
 *   Logs      — live kernel log tail (printk output)
 *
 * Internal name: "pulse"
 * Display name:  "Pulse"
 */

#ifndef PULSE_H
#define PULSE_H

#include <kernel/types.h>

/* ── Snapshot data types ──────────────────────────────────────────────────── */

#define PULSE_GRAPH_POINTS   120   /* 2 minutes at 1 sample/sec */
#define PULSE_MAX_CORES       64
#define PULSE_MAX_PROCESSES   512
#define PULSE_MAX_DISKS        16
#define PULSE_MAX_NICS          8

typedef struct {
    float    usage[PULSE_MAX_CORES];   /* 0.0–100.0 per core */
    float    total_usage;
    uint64_t freq_mhz[PULSE_MAX_CORES];
    int      temp_celsius[PULSE_MAX_CORES];
    uint32_t core_count;
    char     model[128];
} pulse_cpu_t;

typedef struct {
    uint64_t total;       /* bytes */
    uint64_t used;
    uint64_t free;
    uint64_t cached;
    uint64_t swap_total;
    uint64_t swap_used;
} pulse_memory_t;

typedef struct {
    char     name[32];
    uint64_t read_bytes_sec;
    uint64_t write_bytes_sec;
    uint64_t total_reads;
    uint64_t total_writes;
    uint64_t capacity;
    bool     ssd;
} pulse_disk_t;

typedef struct {
    char     iface[16];
    uint64_t rx_bytes_sec;
    uint64_t tx_bytes_sec;
    uint64_t rx_total;
    uint64_t tx_total;
    bool     up;
} pulse_nic_t;

typedef struct {
    pid_t    pid;
    char     name[64];
    float    cpu_percent;
    uint64_t mem_bytes;
    uint32_t threads;
    uint32_t fd_count;
    char     state;     /* R=running, S=sleeping, Z=zombie */
} pulse_process_t;

/* ── Full system snapshot ─────────────────────────────────────────────────── */
typedef struct {
    uint64_t        timestamp;    /* Unix epoch seconds */
    uint64_t        uptime_sec;
    pulse_cpu_t     cpu;
    pulse_memory_t  memory;
    pulse_disk_t    disks[PULSE_MAX_DISKS];
    uint32_t        disk_count;
    pulse_nic_t     nics[PULSE_MAX_NICS];
    uint32_t        nic_count;
    pulse_process_t processes[PULSE_MAX_PROCESSES];
    uint32_t        process_count;
} pulse_snapshot_t;

/* ── Historical ring buffer ───────────────────────────────────────────────── */
typedef struct {
    float    cpu_usage[PULSE_GRAPH_POINTS];
    float    mem_usage[PULSE_GRAPH_POINTS];
    uint64_t net_rx[PULSE_GRAPH_POINTS];
    uint64_t net_tx[PULSE_GRAPH_POINTS];
    uint32_t write_pos;
} pulse_history_t;

/* ── Public API ───────────────────────────────────────────────────────────── */
void pulse_init(void);
void pulse_main(void);
void pulse_collect(pulse_snapshot_t *snap);
void pulse_print_overview(const pulse_snapshot_t *snap);
void pulse_print_processes(const pulse_snapshot_t *snap);

/* Threshold alerts */
void pulse_set_alert_cpu(float threshold_pct);
void pulse_set_alert_mem(float threshold_pct);

#endif /* PULSE_H */
