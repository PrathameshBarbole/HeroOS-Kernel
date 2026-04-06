/*
 * HeroOS Init System (PID 1)
 *
 * Responsibilities:
 *   - Set up the initial filesystem layout
 *   - Start system services (Canvas compositor, Echo, HeroServe, HeroPkg daemon)
 *   - Launch Terminal + HeroShell for the console
 *   - Reap zombie processes
 *   - Handle system shutdown/reboot signals
 */

#include <kernel/types.h>
#include <kernel/printk.h>
#include <kernel/proc/process.h>
#include <kernel/proc/sched.h>
#include <kernel/fs/vfs.h>
#include <lib/string.h>

/* Service descriptor */
typedef struct {
    const char  *name;
    void       (*entry)(void);
    bool         autostart;
    bool         running;
    pid_t        pid;
} service_t;

/* Forward declarations */
static void heroshell_main(void);
static void heroserve_main(void);

static service_t services[] = {
    { "terminal",   terminal_service, true,  false, 0 },
    { "heroserved", heroserve_main,   false, false, 0 },
};

#define SERVICE_COUNT  (sizeof(services) / sizeof(services[0]))

/* ─── Terminal (wraps HeroShell) ─────────────────────────────────────────── */

static void terminal_service(void) {
    pr_info("Terminal: starting console terminal\n");
    printk("\n");
    printk("  ╔══════════════════════════════════════════╗\n");
    printk("  ║     Terminal  ·  HeroOS Console          ║\n");
    printk("  ║     Powered by HeroShell v0.1            ║\n");
    printk("  ╚══════════════════════════════════════════╝\n");
    printk("  Type 'help' for commands. Type 'hero help' for HeroOS tools.\n");
    printk("\n");

    /* HeroShell REPL — implemented in userspace/heroshell/ */
    for (;;) {
        printk("hero$ ");
        /* TODO: read line from keyboard/PTY, parse, execute */
        sched_sleep(1000);
    }
}

/* ─── HeroServe stub ─────────────────────────────────────────────────────── */

static void heroserve_main(void) {
    pr_info("HeroServed: HTTP server starting on port 8080\n");
    /* TODO: implement socket binding and request handling */
    for (;;) sched_sleep(5000);
}

/* ─── Service management ─────────────────────────────────────────────────── */

static void start_service(service_t *svc) {
    if (svc->running) return;
    process_t *p = proc_create_kernel_thread(svc->name, svc->entry);
    if (p) {
        sched_add(p);
        svc->pid     = p->pid;
        svc->running = true;
        pr_info("init: started service '%s' (PID %u)\n", svc->name, svc->pid);
    } else {
        pr_err("init: failed to start service '%s'\n", svc->name);
    }
}

/* ─── Init entry point ───────────────────────────────────────────────────── */

void init_main(void) {
    pr_info("init: PID 1 starting\n");

    /* Start autostart services */
    for (size_t i = 0; i < SERVICE_COUNT; i++) {
        if (services[i].autostart)
            start_service(&services[i]);
    }

    /* Process reaper loop */
    for (;;) {
        /* TODO: waitpid(-1, &status, WNOHANG) to reap zombies */
        sched_sleep(100);
    }
}
