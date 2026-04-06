#include <kernel/ipc/ipc.h>
#include <kernel/mm/kheap.h>
#include <kernel/mm/pmm.h>
#include <kernel/printk.h>
#include <lib/string.h>

/* ─── Signals ────────────────────────────────────────────────────────────── */

static signal_handler_t signal_handlers[NSIG];

void signal_set_handler(int signo, signal_handler_t handler) {
    if (signo > 0 && signo < NSIG)
        signal_handlers[signo] = handler;
}

void signal_send(pid_t pid, int signo) {
    extern process_t *proc_get(pid_t pid);
    process_t *proc = proc_get(pid);
    if (!proc) return;

    if (signo == SIGKILL || signo == SIGTERM) {
        extern void proc_exit(process_t *, int);
        proc_exit(proc, -signo);
        return;
    }
    if (signo == SIGSTOP) { proc->state = PROC_BLOCKED; return; }
    if (signo == SIGCONT) {
        if (proc->state == PROC_BLOCKED) proc->state = PROC_READY;
        return;
    }
    pr_debug("signal_send: PID %u signal %d\n", pid, signo);
}

void signal_handle_pending(void) {
    /* Placeholder — full async signal delivery in a future milestone */
}

/* ─── Pipes ──────────────────────────────────────────────────────────────── */

pipe_t *pipe_create(void) {
    pipe_t *p = (pipe_t *)kcalloc(1, sizeof(pipe_t));
    return p;
}

void pipe_destroy(pipe_t *p) {
    kfree(p);
}

ssize_t pipe_read(pipe_t *p, void *buf, size_t count) {
    if (!p || p->read_closed) return -1;
    if (p->bytes_available == 0) {
        if (p->write_closed) return 0;   /* EOF */
        return -11;                      /* EAGAIN */
    }
    size_t to_read = MIN(count, p->bytes_available);
    for (size_t i = 0; i < to_read; i++) {
        ((uint8_t *)buf)[i] = (uint8_t)p->buf[p->read_pos % PIPE_BUF_SIZE];
        p->read_pos++;
        p->bytes_available--;
    }
    return (ssize_t)to_read;
}

ssize_t pipe_write(pipe_t *p, const void *buf, size_t count) {
    if (!p || p->write_closed) return -1;
    size_t space = PIPE_BUF_SIZE - p->bytes_available;
    size_t to_write = MIN(count, space);
    if (to_write == 0) return -11;   /* EAGAIN — pipe full */
    for (size_t i = 0; i < to_write; i++) {
        p->buf[p->write_pos % PIPE_BUF_SIZE] = (char)((const uint8_t *)buf)[i];
        p->write_pos++;
        p->bytes_available++;
    }
    return (ssize_t)to_write;
}

/* ─── Message Queues ──────────────────────────────────────────────────────── */

static msg_queue_t *msg_queues[64];
static int next_qid = 0;

int msgq_create(void) {
    if (next_qid >= 64) return -1;
    msg_queues[next_qid] = (msg_queue_t *)kcalloc(1, sizeof(msg_queue_t));
    if (!msg_queues[next_qid]) return -1;
    msg_queues[next_qid]->id = next_qid;
    return next_qid++;
}

void msgq_destroy(int qid) {
    if (qid < 0 || qid >= 64 || !msg_queues[qid]) return;
    kfree(msg_queues[qid]);
    msg_queues[qid] = NULL;
}

int msgq_send(int qid, const message_t *msg) {
    if (qid < 0 || qid >= 64 || !msg_queues[qid]) return -1;
    msg_queue_t *q = msg_queues[qid];
    if (q->count >= MSG_QUEUE_LEN) return -11;   /* EAGAIN */
    memcpy(&q->messages[q->tail], msg, sizeof(message_t));
    q->tail = (q->tail + 1) % MSG_QUEUE_LEN;
    q->count++;
    return 0;
}

int msgq_recv(int qid, message_t *msg) {
    if (qid < 0 || qid >= 64 || !msg_queues[qid]) return -1;
    msg_queue_t *q = msg_queues[qid];
    if (q->count == 0) return -11;   /* EAGAIN */
    memcpy(msg, &q->messages[q->head], sizeof(message_t));
    q->head = (q->head + 1) % MSG_QUEUE_LEN;
    q->count--;
    return 0;
}

/* ─── Shared Memory ───────────────────────────────────────────────────────── */

static shm_region_t shm_regions[SHM_MAX_SEGS];
static int next_shmid = 0;

int shm_create(size_t size) {
    if (next_shmid >= SHM_MAX_SEGS) return -1;
    size = ALIGN_UP(size, PAGE_SIZE);
    if (size > SHM_MAX_SIZE) return -1;

    uintptr_t phys = pmm_alloc_pages(size / PAGE_SIZE);
    if (!phys) return -1;

    shm_regions[next_shmid].id        = next_shmid;
    shm_regions[next_shmid].size      = size;
    shm_regions[next_shmid].phys_base = phys;
    shm_regions[next_shmid].ref_count = 0;

    return next_shmid++;
}

void *shm_attach(int shmid, addr_space_t *as) {
    if (shmid < 0 || shmid >= SHM_MAX_SEGS) return NULL;
    shm_region_t *r = &shm_regions[shmid];
    if (!r->phys_base) return NULL;

    /* Map into address space */
    extern int vmm_map_range(addr_space_t *, uintptr_t, uintptr_t, size_t, uint64_t);
    extern uintptr_t vmm_alloc_pages(addr_space_t *, size_t, uint64_t);
    (void)as;

    r->ref_count++;
    return (void *)r->phys_base;   /* Simplified: return phys directly */
}

void shm_detach(int shmid, addr_space_t *as, void *addr) {
    (void)as; (void)addr;
    if (shmid < 0 || shmid >= SHM_MAX_SEGS) return;
    if (shm_regions[shmid].ref_count > 0)
        shm_regions[shmid].ref_count--;
}

void shm_destroy(int shmid) {
    if (shmid < 0 || shmid >= SHM_MAX_SEGS) return;
    shm_region_t *r = &shm_regions[shmid];
    if (r->phys_base) {
        pmm_free_pages(r->phys_base, r->size / PAGE_SIZE);
        r->phys_base = 0;
    }
}
