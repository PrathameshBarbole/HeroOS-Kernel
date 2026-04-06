#ifndef KERNEL_SYSCALL_H
#define KERNEL_SYSCALL_H

#include <kernel/types.h>
#include <kernel/proc/process.h>

/*
 * HeroOS System Call Table
 * Designed to be a minimal Linux-compatible subset, enabling easy porting of
 * developer tools (git, Python, Node.js, etc.) without full ABI emulation.
 *
 * Calling convention (x86_64):
 *   rax = syscall number
 *   rdi, rsi, rdx, r10, r8, r9 = arguments 1-6
 *   rax = return value (negative = error code)
 */

/* ── Process ─────────────────────────── */
#define SYS_exit          1
#define SYS_fork          2
#define SYS_exec          3
#define SYS_getpid        4
#define SYS_getppid       5
#define SYS_waitpid       6
#define SYS_sleep         7
#define SYS_yield         8
#define SYS_kill          9
#define SYS_signal        10

/* ── File System ─────────────────────── */
#define SYS_open          20
#define SYS_close         21
#define SYS_read          22
#define SYS_write         23
#define SYS_seek          24
#define SYS_stat          25
#define SYS_mkdir         26
#define SYS_rmdir         27
#define SYS_unlink        28
#define SYS_rename        29
#define SYS_readdir       30
#define SYS_chdir         31
#define SYS_getcwd        32
#define SYS_dup           33
#define SYS_dup2          34
#define SYS_pipe          35
#define SYS_ioctl         36

/* ── Memory ──────────────────────────── */
#define SYS_mmap          40
#define SYS_munmap        41
#define SYS_brk           42
#define SYS_mprotect      43

/* ── IPC ─────────────────────────────── */
#define SYS_msgq_create   50
#define SYS_msgq_send     51
#define SYS_msgq_recv     52
#define SYS_shm_create    53
#define SYS_shm_attach    54
#define SYS_shm_detach    55

/* ── Networking (future) ─────────────── */
#define SYS_socket        60
#define SYS_bind          61
#define SYS_listen        62
#define SYS_accept        63
#define SYS_connect       64
#define SYS_send          65
#define SYS_recv          66

/* ── System info ─────────────────────── */
#define SYS_uname         80
#define SYS_uptime        81
#define SYS_sysinfo       82
#define SYS_time          83

#define SYSCALL_COUNT     128

/* Error codes */
#define ESUCCESS   0
#define EPERM      1
#define ENOENT     2
#define ESRCH      3
#define EINTR      4
#define EIO        5
#define ENXIO      6
#define EBADF      9
#define ECHILD     10
#define EAGAIN     11
#define ENOMEM     12
#define EACCES     13
#define EFAULT     14
#define EBUSY      16
#define EEXIST     17
#define EISDIR     21
#define EINVAL     22
#define ENOSPC     28
#define ESPIPE     29
#define ERANGE     34
#define ENOSYS     38
#define ENOTEMPTY  39

typedef int64_t (*syscall_fn_t)(uint64_t a1, uint64_t a2, uint64_t a3,
                                 uint64_t a4, uint64_t a5, uint64_t a6);

void    syscall_init(void);
int64_t syscall_dispatch(uint64_t num,
                          uint64_t a1, uint64_t a2, uint64_t a3,
                          uint64_t a4, uint64_t a5, uint64_t a6);

/* Syscall entry point called from ISR (int 0x80) */
void syscall_handler(void *frame);

#endif /* KERNEL_SYSCALL_H */
