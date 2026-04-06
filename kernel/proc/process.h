#ifndef KERNEL_PROCESS_H
#define KERNEL_PROCESS_H

#include <kernel/types.h>
#include <kernel/mm/vmm.h>

/* Process states */
typedef enum {
    PROC_CREATED   = 0,
    PROC_READY     = 1,
    PROC_RUNNING   = 2,
    PROC_BLOCKED   = 3,
    PROC_SLEEPING  = 4,
    PROC_ZOMBIE    = 5,
    PROC_DEAD      = 6,
} proc_state_t;

/* Process flags */
#define PROC_FLAG_KERNEL   BIT(0)  /* Kernel thread */
#define PROC_FLAG_USER     BIT(1)  /* User-space process */

#define PROC_NAME_LEN   64
#define MAX_PROCESSES   1024
#define KERNEL_STACK_SIZE  (16 * 1024)   /* 16 KiB kernel stack per process */

/* Saved CPU context for context switching */
typedef struct {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t rip;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t cs;
    uint64_t ss;
} cpu_context_t;

/* File descriptor table (stub) */
#define MAX_FDS  256

/* Process Control Block (PCB) */
typedef struct process {
    pid_t           pid;
    pid_t           ppid;              /* Parent PID */
    char            name[PROC_NAME_LEN];
    proc_state_t    state;
    uint32_t        flags;

    cpu_context_t   context;           /* Saved CPU registers */
    addr_space_t   *addr_space;        /* Virtual address space */

    uintptr_t       kernel_stack;      /* Kernel stack (virtual) */
    uintptr_t       kernel_stack_phys;
    size_t          kernel_stack_size;

    int             exit_code;
    uint64_t        runtime_ticks;     /* CPU ticks consumed */
    uint64_t        sleep_until;       /* Wake-up tick (for SLEEPING state) */

    uint32_t        priority;          /* Scheduler priority (0=highest) */
    uint32_t        vruntime;          /* Virtual runtime for CFS */

    /* File descriptor table */
    void           *fds[MAX_FDS];

    /* Linked list pointers */
    struct process *next;
    struct process *prev;
} process_t;

/* Process management API */
void       proc_init(void);
process_t *proc_create(const char *name, void (*entry)(void), uint32_t flags, uint32_t priority);
process_t *proc_create_kernel_thread(const char *name, void (*entry)(void));
void       proc_exit(process_t *proc, int exit_code);
void       proc_kill(pid_t pid, int sig);
process_t *proc_get(pid_t pid);
process_t *proc_current(void);
void       proc_dump_all(void);

/* Context switch (defined in switch.asm) */
extern void context_switch(cpu_context_t *old_ctx, cpu_context_t *new_ctx);

#endif /* KERNEL_PROCESS_H */
