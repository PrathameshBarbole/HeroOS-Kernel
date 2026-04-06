#ifndef KERNEL_KHEAP_H
#define KERNEL_KHEAP_H

#include <kernel/types.h>

/* Kernel heap public API — slab-backed buddy allocator */
void  kheap_init(uintptr_t start, size_t size);
void *kmalloc(size_t size);
void *kcalloc(size_t count, size_t size);
void *krealloc(void *ptr, size_t new_size);
void  kfree(void *ptr);

/* Aligned allocation (align must be power-of-two) */
void *kmalloc_aligned(size_t size, size_t align);

/* Debug: print heap statistics */
void kheap_dump_stats(void);

#endif /* KERNEL_KHEAP_H */
