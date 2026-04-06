#ifndef KERNEL_PMM_H
#define KERNEL_PMM_H

#include <kernel/types.h>

/* Multiboot2 memory map structures (passed from bootloader) */
#define MULTIBOOT2_MAGIC          0x36D76289U
#define MB2_TAG_TYPE_MMAP         6
#define MB2_MMAP_TYPE_AVAILABLE   1
#define MB2_MMAP_TYPE_RESERVED    2
#define MB2_MMAP_TYPE_ACPI_RECLM  3
#define MB2_MMAP_TYPE_NVS         4
#define MB2_MMAP_TYPE_BADRAM      5

typedef struct PACKED {
    uint32_t type;
    uint32_t size;
} mb2_tag_t;

typedef struct PACKED {
    uint64_t base_addr;
    uint64_t length;
    uint32_t type;
    uint32_t reserved;
} mb2_mmap_entry_t;

typedef struct PACKED {
    uint32_t type;           /* MB2_TAG_TYPE_MMAP = 6 */
    uint32_t size;
    uint32_t entry_size;
    uint32_t entry_version;
    mb2_mmap_entry_t entries[];
} mb2_mmap_tag_t;

typedef struct PACKED {
    uint32_t total_size;
    uint32_t reserved;
} mb2_info_t;

/* PMM statistics */
typedef struct {
    uint64_t total_pages;
    uint64_t free_pages;
    uint64_t used_pages;
    uint64_t reserved_pages;
} pmm_stats_t;

/* PMM public API */
void      pmm_init(uintptr_t mb2_info_phys);
uintptr_t pmm_alloc_page(void);
uintptr_t pmm_alloc_pages(size_t count);
void      pmm_free_page(uintptr_t phys);
void      pmm_free_pages(uintptr_t phys, size_t count);
void      pmm_mark_used(uintptr_t phys, size_t size);
void      pmm_mark_free(uintptr_t phys, size_t size);
void      pmm_get_stats(pmm_stats_t *stats);

/* Total physical memory in bytes */
uint64_t  pmm_total_memory(void);

#endif /* KERNEL_PMM_H */
