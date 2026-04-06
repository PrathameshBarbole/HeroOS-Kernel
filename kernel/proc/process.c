#include <kernel/proc/process.h>
#include <kernel/proc/sched.h>
#include <kernel/mm/kheap.h>
#include <kernel/mm/pmm.h>
#include <kernel/mm/vmm.h>
#include <kernel/printk.h>
#include <lib/string.h>

/* ─── Process table ──────────────────────────────────────────────────────── */

static process_t *proc_table[MAX_PROCESSES];
static pid_t      next_pid = 1;

/* Idle process (PID 0) */
static process_t idle_proc;

/* ─── Idle task ──────────────────────────────────────────────────────────── */

static void idle_task(void) {
    for (;;) __asm__ volatile("hlt");
}

/* ─── Process creation ───────────────────────────────────────────────────── */

void proc_init(void) {
    memset(proc_table, 0, sizeof(proc_table));

    /* Create idle process (PID 0) */
    memset(&idle_proc, 0, sizeof(idle_proc));
    idle_proc.pid      = 0;
    idle_proc.ppid     = 0;
    idle_proc.state    = PROC_READY;
    idle_proc.flags    = PROC_FLAG_KERNEL;
    idle_proc.priority = 255;   /* Lowest priority */
    strncpy(idle_proc.name, "[idle]", PROC_NAME_LEN - 1);

    idle_proc.kernel_stack_size = KERNEL_STACK_SIZE;
    idle_proc.kernel_stack_phys = pmm_alloc_pages(KERNEL_STACK_SIZE / PAGE_SIZE);
    idle_proc.kernel_stack      = idle_proc.kernel_stack_phys;  /* Identity-mapped */

    /* Set up idle task context */
    memset(&idle_proc.context, 0, sizeof(cpu_context_t));
    idle_proc.context.rip    = (uint64_t)(uintptr_t)idle_task;
    idle_proc.context.rsp    = idle_proc.kernel_stack + KERNEL_STACK_SIZE;
    idle_proc.context.cs     = 0x08;
    idle_proc.context.ss     = 0x10;
    idle_proc.context.rflags = 0x202;   /* IF=1 */

    proc_table[0] = &idle_proc;
    pr_info("Process manager initialised (idle PID 0)\n");
}

process_t *proc_create(const char *name, void (*entry)(void),
                       uint32_t flags, uint32_t priority) {
    /* Find free slot */
    pid_t pid = -1;
    for (pid_t i = 1; i < MAX_PROCESSES; i++) {
        if (!proc_table[i]) { pid = i; break; }
    }
    if (pid < 0) {
        pr_err("proc_create: process table full\n");
        return NULL;
    }

    process_t *proc = (process_t *)kcalloc(1, sizeof(process_t));
    if (!proc) return NULL;

    proc->pid      = (pid_t)pid;
    proc->ppid     = proc_current() ? proc_current()->pid : 0;
    proc->state    = PROC_CREATED;
    proc->flags    = flags;
    proc->priority = priority;
    strncpy(proc->name, name ? name : "unnamed", PROC_NAME_LEN - 1);

    /* Allocate kernel stack */
    proc->kernel_stack_size = KERNEL_STACK_SIZE;
    proc->kernel_stack_phys = pmm_alloc_pages(KERNEL_STACK_SIZE / PAGE_SIZE);
    proc->kernel_stack      = proc->kernel_stack_phys;   /* Identity-mapped */

    /* Create address space */
    proc->addr_space = vmm_create_address_space();

    /* Set up initial CPU context */
    memset(&proc->context, 0, sizeof(cpu_context_t));
    proc->context.rip    = (uint64_t)(uintptr_t)entry;
    proc->context.rsp    = proc->kernel_stack + KERNEL_STACK_SIZE;
    proc->context.cs     = (flags & PROC_FLAG_KERNEL) ? 0x08 : 0x1B;
    proc->context.ss     = (flags & PROC_FLAG_KERNEL) ? 0x10 : 0x23;
    proc->context.rflags = 0x202;   /* IF=1, reserved bit 1 */

    proc->state = PROC_READY;
    proc_table[pid] = proc;
    next_pid = (pid_t)(pid + 1);

    pr_debug("Created process '%s' (PID %u, priority %u)\n",
             proc->name, proc->pid, proc->priority);
    return proc;
}

process_t *proc_create_kernel_thread(const char *name, void (*entry)(void)) {
    return proc_create(name, entry, PROC_FLAG_KERNEL, 10);
}

void proc_exit(process_t *proc, int exit_code) {
    if (!proc) return;
    proc->exit_code = exit_code;
    proc->state     = PROC_ZOMBIE;
    pr_info("Process '%s' (PID %u) exited with code %d\n",
            proc->name, proc->pid, exit_code);
    sched_yield();
}

void proc_kill(pid_t pid, int sig) {
    (void)sig;
    process_t *proc = proc_get(pid);
    if (proc) proc_exit(proc, -1);
}

process_t *proc_get(pid_t pid) {
    if (pid >= MAX_PROCESSES) return NULL;
    return proc_table[pid];
}

process_t *proc_current(void) {
    return g_runqueue.current;
}

void proc_dump_all(void) {
    printk("PID  STATE      PRI  NAME\n");
    printk("---- ---------- ---- ----------------\n");
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (!proc_table[i]) continue;
        process_t *p = proc_table[i];
        const char *states[] = {
            "CREATED", "READY", "RUNNING", "BLOCKED", "SLEEPING", "ZOMBIE", "DEAD"
        };
        const char *state = (p->state <= PROC_DEAD) ? states[p->state] : "UNKNOWN";
        printk("%-4u %-10s %-4u %s\n", p->pid, state, p->priority, p->name);
    }
}
