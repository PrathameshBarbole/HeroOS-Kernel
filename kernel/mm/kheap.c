#include <kernel/mm/kheap.h>
#include <kernel/mm/pmm.h>
#include <kernel/printk.h>
#include <lib/string.h>

/*
 * Kernel heap — simple free-list allocator (first-fit) backed by the PMM.
 *
 * Each allocation is prefixed by an alloc_header_t that records the block
 * size and whether it is free.  A footer (alloc_footer_t) allows adjacent
 * free blocks to be coalesced efficiently.
 */

#define HEAP_MAGIC_FREE  0xDEADBEEFULL
#define HEAP_MAGIC_USED  0xCAFEBABEULL
#define HEAP_MIN_SIZE    (PAGE_SIZE * 16)   /* 64 KiB initial heap */

typedef struct alloc_header {
    uint64_t magic;
    size_t   size;          /* Usable payload size (not including headers) */
    bool     free;
    struct alloc_header *prev;
    struct alloc_header *next;
} alloc_header_t;

typedef struct {
    uint64_t magic;
    alloc_header_t *header;  /* Back-pointer to matching header */
} alloc_footer_t;

#define HEADER_SIZE  (sizeof(alloc_header_t))
#define FOOTER_SIZE  (sizeof(alloc_footer_t))
#define OVERHEAD     (HEADER_SIZE + FOOTER_SIZE)

static alloc_header_t *heap_head = NULL;
static uintptr_t heap_base = 0;
static uintptr_t heap_end  = 0;
static uintptr_t heap_max  = 0;

/* ─── Internal helpers ───────────────────────────────────────────────────── */

static alloc_footer_t *get_footer(alloc_header_t *h) {
    return (alloc_footer_t *)((uint8_t *)h + HEADER_SIZE + h->size);
}

static void write_block(alloc_header_t *h, size_t size, bool free) {
    h->magic = free ? HEAP_MAGIC_FREE : HEAP_MAGIC_USED;
    h->size  = size;
    h->free  = free;

    alloc_footer_t *f = get_footer(h);
    f->magic  = free ? HEAP_MAGIC_FREE : HEAP_MAGIC_USED;
    f->header = h;
}

/* Expand the heap by allocating more physical pages */
static int heap_expand(size_t bytes_needed) {
    size_t pages = ALIGN_UP(bytes_needed, PAGE_SIZE) / PAGE_SIZE;
    uintptr_t new_pages_phys = pmm_alloc_pages(pages);

    /* Identity-map or use existing mapping for kernel heap */
    uintptr_t new_end = heap_end + pages * PAGE_SIZE;
    if (new_end > heap_max) return -1;

    heap_end = new_end;

    /* Create a free block from the newly acquired region */
    alloc_header_t *block = (alloc_header_t *)((uintptr_t)new_pages_phys);
    write_block(block, pages * PAGE_SIZE - OVERHEAD, true);
    block->prev = NULL;
    block->next = NULL;

    /* Append to free list */
    if (!heap_head) {
        heap_head = block;
    } else {
        alloc_header_t *cur = heap_head;
        while (cur->next) cur = cur->next;
        cur->next = block;
        block->prev = cur;
    }
    return 0;
}

/* ─── Public API ─────────────────────────────────────────────────────────── */

void kheap_init(uintptr_t start, size_t size) {
    heap_base = start;
    heap_end  = start;
    heap_max  = start + size;

    /* Pre-allocate initial region */
    size_t init_size = MIN(HEAP_MIN_SIZE, size);
    if (heap_expand(init_size) != 0)
        kernel_panic("kheap_init: failed to allocate initial heap pages");

    pr_info("Heap initialised: base=0x%llx, initial=%zu KiB\n",
            (uint64_t)heap_base, init_size / 1024);
}

void *kmalloc(size_t size) {
    if (size == 0) return NULL;

    /* Align size to 16 bytes for proper alignment */
    size = ALIGN_UP(size, 16);

    /* First-fit search */
    alloc_header_t *cur = heap_head;
    while (cur) {
        if (cur->free && cur->size >= size + OVERHEAD) {
            /* Split the block if remainder is large enough */
            if (cur->size >= size + OVERHEAD + 32) {
                size_t old_size = cur->size;
                write_block(cur, size, false);

                alloc_header_t *next_block = (alloc_header_t *)
                    ((uint8_t *)cur + HEADER_SIZE + size + FOOTER_SIZE);
                write_block(next_block, old_size - size - OVERHEAD, true);
                next_block->prev = cur;
                next_block->next = cur->next;
                if (cur->next) cur->next->prev = next_block;
                cur->next = next_block;
            } else {
                write_block(cur, cur->size, false);
            }
            return (void *)((uint8_t *)cur + HEADER_SIZE);
        }
        cur = cur->next;
    }

    /* No suitable block — expand heap and retry */
    if (heap_expand(size + OVERHEAD + PAGE_SIZE) == 0)
        return kmalloc(size);

    pr_err("kmalloc: failed to allocate %zu bytes\n", size);
    return NULL;
}

void *kcalloc(size_t count, size_t size) {
    void *ptr = kmalloc(count * size);
    if (ptr) memset(ptr, 0, count * size);
    return ptr;
}

void kfree(void *ptr) {
    if (!ptr) return;

    alloc_header_t *h = (alloc_header_t *)((uint8_t *)ptr - HEADER_SIZE);
    if (h->magic != HEAP_MAGIC_USED) {
        pr_err("kfree: bad magic at %p (double-free or corruption?)\n", ptr);
        return;
    }

    write_block(h, h->size, true);

    /* Coalesce with next block */
    if (h->next && h->next->free) {
        alloc_header_t *n = h->next;
        h->size += OVERHEAD + n->size;
        h->next  = n->next;
        if (n->next) n->next->prev = h;
        get_footer(h)->header = h;
    }

    /* Coalesce with prev block */
    if (h->prev && h->prev->free) {
        alloc_header_t *p = h->prev;
        p->size += OVERHEAD + h->size;
        p->next  = h->next;
        if (h->next) h->next->prev = p;
        get_footer(p)->header = p;
    }
}

void *krealloc(void *ptr, size_t new_size) {
    if (!ptr) return kmalloc(new_size);
    if (new_size == 0) { kfree(ptr); return NULL; }

    alloc_header_t *h = (alloc_header_t *)((uint8_t *)ptr - HEADER_SIZE);
    if (h->size >= new_size) return ptr;   /* Already big enough */

    void *new_ptr = kmalloc(new_size);
    if (!new_ptr) return NULL;
    memcpy(new_ptr, ptr, h->size);
    kfree(ptr);
    return new_ptr;
}

void *kmalloc_aligned(size_t size, size_t align) {
    /* Simple implementation: allocate extra, manually align */
    void *raw = kmalloc(size + align + sizeof(void *));
    if (!raw) return NULL;
    uintptr_t aligned = ALIGN_UP((uintptr_t)raw + sizeof(void *), align);
    ((void **)aligned)[-1] = raw;
    return (void *)aligned;
}

void kheap_dump_stats(void) {
    size_t free_bytes = 0, used_bytes = 0, blocks = 0;
    alloc_header_t *cur = heap_head;
    while (cur) {
        blocks++;
        if (cur->free) free_bytes += cur->size;
        else           used_bytes += cur->size;
        cur = cur->next;
    }
    printk("Heap stats: %zu blocks, %zu KiB used, %zu KiB free\n",
           blocks, used_bytes / 1024, free_bytes / 1024);
}
