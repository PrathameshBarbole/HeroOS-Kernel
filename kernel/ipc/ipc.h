#ifndef KERNEL_IPC_H
#define KERNEL_IPC_H

#include <kernel/types.h>
#include <kernel/proc/process.h>

/* ── Signals ─────────────────────────────────────────── */
#define NSIG          32

#define SIGHUP        1
#define SIGINT        2
#define SIGQUIT       3
#define SIGILL        4
#define SIGTRAP       5
#define SIGABRT       6
#define SIGFPE        8
#define SIGKILL       9
#define SIGSEGV       11
#define SIGPIPE       13
#define SIGALRM       14
#define SIGTERM       15
#define SIGCHLD       17
#define SIGCONT       18
#define SIGSTOP       19
#define SIGTSTP       20

typedef void (*signal_handler_t)(int signo);
#define SIG_DFL  ((signal_handler_t)0)
#define SIG_IGN  ((signal_handler_t)1)

void signal_send(pid_t pid, int signo);
void signal_handle_pending(void);
void signal_set_handler(int signo, signal_handler_t handler);

/* ── Pipes ────────────────────────────────────────────── */
#define PIPE_BUF_SIZE  4096

typedef struct {
    char     buf[PIPE_BUF_SIZE];
    size_t   read_pos;
    size_t   write_pos;
    size_t   bytes_available;
    bool     write_closed;
    bool     read_closed;
} pipe_t;

pipe_t *pipe_create(void);
void    pipe_destroy(pipe_t *p);
ssize_t pipe_read(pipe_t *p, void *buf, size_t count);
ssize_t pipe_write(pipe_t *p, const void *buf, size_t count);

/* ── Message Queues ──────────────────────────────────── */
#define MSG_MAX_SIZE   1024
#define MSG_QUEUE_LEN  64

typedef struct {
    pid_t   sender;
    size_t  size;
    char    data[MSG_MAX_SIZE];
} message_t;

typedef struct {
    int        id;
    message_t  messages[MSG_QUEUE_LEN];
    size_t     head;
    size_t     tail;
    size_t     count;
} msg_queue_t;

int      msgq_create(void);
void     msgq_destroy(int qid);
int      msgq_send(int qid, const message_t *msg);
int      msgq_recv(int qid, message_t *msg);

/* ── Shared Memory ───────────────────────────────────── */
#define SHM_MAX_SIZE   (1024 * 1024)   /* 1 MiB per shared region */
#define SHM_MAX_SEGS   64

typedef struct {
    int        id;
    size_t     size;
    uintptr_t  phys_base;
    int        ref_count;
} shm_region_t;

int       shm_create(size_t size);
void     *shm_attach(int shmid, addr_space_t *as);
void      shm_detach(int shmid, addr_space_t *as, void *addr);
void      shm_destroy(int shmid);

#endif /* KERNEL_IPC_H */
