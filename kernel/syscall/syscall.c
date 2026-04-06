#include <kernel/syscall/syscall.h>
#include <kernel/proc/process.h>
#include <kernel/proc/sched.h>
#include <kernel/fs/vfs.h>
#include <kernel/mm/vmm.h>
#include <kernel/ipc/ipc.h>
#include <kernel/printk.h>
#include <lib/string.h>

/* ─── Syscall dispatch table ─────────────────────────────────────────────── */

static syscall_fn_t syscall_table[SYSCALL_COUNT];

/* ─── Process syscalls ───────────────────────────────────────────────────── */

static int64_t sys_exit(uint64_t code, uint64_t a2, uint64_t a3,
                        uint64_t a4, uint64_t a5, uint64_t a6) {
    (void)a2; (void)a3; (void)a4; (void)a5; (void)a6;
    process_t *cur = proc_current();
    if (cur) proc_exit(cur, (int)code);
    sched_yield();
    return 0;
}

static int64_t sys_getpid(uint64_t a1, uint64_t a2, uint64_t a3,
                          uint64_t a4, uint64_t a5, uint64_t a6) {
    (void)a1; (void)a2; (void)a3; (void)a4; (void)a5; (void)a6;
    process_t *cur = proc_current();
    return cur ? (int64_t)cur->pid : 0;
}

static int64_t sys_getppid(uint64_t a1, uint64_t a2, uint64_t a3,
                           uint64_t a4, uint64_t a5, uint64_t a6) {
    (void)a1; (void)a2; (void)a3; (void)a4; (void)a5; (void)a6;
    process_t *cur = proc_current();
    return cur ? (int64_t)cur->ppid : 0;
}

static int64_t sys_yield(uint64_t a1, uint64_t a2, uint64_t a3,
                         uint64_t a4, uint64_t a5, uint64_t a6) {
    (void)a1; (void)a2; (void)a3; (void)a4; (void)a5; (void)a6;
    sched_yield();
    return 0;
}

static int64_t sys_sleep(uint64_t ms, uint64_t a2, uint64_t a3,
                         uint64_t a4, uint64_t a5, uint64_t a6) {
    (void)a2; (void)a3; (void)a4; (void)a5; (void)a6;
    sched_sleep(ms);
    return 0;
}

static int64_t sys_kill(uint64_t pid, uint64_t sig, uint64_t a3,
                        uint64_t a4, uint64_t a5, uint64_t a6) {
    (void)a3; (void)a4; (void)a5; (void)a6;
    signal_send((pid_t)pid, (int)sig);
    return 0;
}

/* ─── File syscalls ───────────────────────────────────────────────────────── */

static int64_t sys_write(uint64_t fd, uint64_t buf_ptr, uint64_t count,
                         uint64_t a4, uint64_t a5, uint64_t a6) {
    (void)a4; (void)a5; (void)a6;
    /* fd 1 = stdout, fd 2 = stderr → write to serial */
    if (fd == 1 || fd == 2) {
        const char *s = (const char *)(uintptr_t)buf_ptr;
        size_t n = (size_t)count;
        for (size_t i = 0; i < n; i++) {
            extern void serial_write_char(char c);
            serial_write_char(s[i]);
        }
        return (int64_t)count;
    }
    /* TODO: VFS write for other fds */
    return -EBADF;
}

static int64_t sys_read(uint64_t fd, uint64_t buf_ptr, uint64_t count,
                        uint64_t a4, uint64_t a5, uint64_t a6) {
    (void)a4; (void)a5; (void)a6;
    (void)fd; (void)buf_ptr; (void)count;
    /* TODO: VFS read */
    return -ENOSYS;
}

/* ─── System info ────────────────────────────────────────────────────────── */

typedef struct {
    char sysname[32];
    char nodename[32];
    char release[32];
    char version[64];
    char machine[32];
} uname_t;

static int64_t sys_uname(uint64_t buf_ptr, uint64_t a2, uint64_t a3,
                         uint64_t a4, uint64_t a5, uint64_t a6) {
    (void)a2; (void)a3; (void)a4; (void)a5; (void)a6;
    uname_t *u = (uname_t *)(uintptr_t)buf_ptr;
    if (!u) return -EFAULT;
    strncpy(u->sysname,  "HeroOS",   31);
    strncpy(u->nodename, "heroos",   31);
    strncpy(u->release,  "0.1.0",    31);
    strncpy(u->version,  "#1 SMP",   63);
    strncpy(u->machine,  "x86_64",   31);
    return 0;
}

/* ─── Dispatch ───────────────────────────────────────────────────────────── */

void syscall_init(void) {
    memset(syscall_table, 0, sizeof(syscall_table));

    syscall_table[SYS_exit]    = sys_exit;
    syscall_table[SYS_getpid]  = sys_getpid;
    syscall_table[SYS_getppid] = sys_getppid;
    syscall_table[SYS_yield]   = sys_yield;
    syscall_table[SYS_sleep]   = sys_sleep;
    syscall_table[SYS_kill]    = sys_kill;
    syscall_table[SYS_write]   = sys_write;
    syscall_table[SYS_read]    = sys_read;
    syscall_table[SYS_uname]   = sys_uname;

    pr_info("Syscall interface initialised (%d entries)\n", SYSCALL_COUNT);
}

int64_t syscall_dispatch(uint64_t num,
                          uint64_t a1, uint64_t a2, uint64_t a3,
                          uint64_t a4, uint64_t a5, uint64_t a6) {
    if (num >= SYSCALL_COUNT || !syscall_table[num]) {
        pr_warn("syscall_dispatch: unknown syscall %llu\n", num);
        return -ENOSYS;
    }
    return syscall_table[num](a1, a2, a3, a4, a5, a6);
}

/* Called from ISR (int 0x80) — frame->rax = syscall number */
void syscall_handler(void *frame_ptr) {
    interrupt_frame_t *frame = (interrupt_frame_t *)frame_ptr;
    int64_t ret = syscall_dispatch(frame->rax,
                                   frame->rdi, frame->rsi, frame->rdx,
                                   frame->r10, frame->r8,  frame->r9);
    frame->rax = (uint64_t)ret;
}
