#include <kernel/mm/pmm.h>
#include <kernel/printk.h>
#include <lib/string.h>

/* ─── Bitmap-based physical memory manager ───────────────────────────────── */
/*
 * Each bit in the bitmap represents one 4 KiB physical page.
 * Bit = 0 → free,  bit = 1 → used/reserved.
 *
 * The bitmap itself is stored in the first available physical RAM region
 * large enough to hold it (typically just above the kernel).
 */

#define PMM_MAX_PAGES    (1024 * 1024)    /* Support up to 4 GiB physical RAM */
#define BITMAP_WORDS     (PMM_MAX_PAGES / 64)

static uint64_t pmm_bitmap[BITMAP_WORDS];   /* 512 KiB bitmap in BSS */
static uint64_t pmm_total_phys_pages = 0;
static uint64_t pmm_free_phys_pages  = 0;
static uint64_t pmm_last_free_hint   = 0;   /* Allocation hint for speed */

/* ─── Bitmap helpers ─────────────────────────────────────────────────────── */

static void bitmap_set(uint64_t page) {
    pmm_bitmap[page / 64] |=  (1ULL << (page % 64));
}

static void bitmap_clear(uint64_t page) {
    pmm_bitmap[page / 64] &= ~(1ULL << (page % 64));
}

static bool bitmap_test(uint64_t page) {
    return (pmm_bitmap[page / 64] & (1ULL << (page % 64))) != 0;
}

/* ─── PMM initialisation ─────────────────────────────────────────────────── */

void pmm_init(uintptr_t mb2_info_phys) {
    /* Start with all pages marked as used */
    memset(pmm_bitmap, 0xFF, sizeof(pmm_bitmap));
    pmm_total_phys_pages = 0;
    pmm_free_phys_pages  = 0;

    if (mb2_info_phys == 0) {
        pr_warn("PMM: no multiboot2 info — physical memory detection skipped\n");
        return;
    }

    /* Parse multiboot2 tags to find the memory map */
    mb2_info_t *mb2 = (mb2_info_t *)(uintptr_t)mb2_info_phys;
    uint8_t *tag_ptr = (uint8_t *)mb2 + 8;
    uint8_t *end_ptr = (uint8_t *)mb2 + mb2->total_size;

    while (tag_ptr < end_ptr) {
        mb2_tag_t *tag = (mb2_tag_t *)tag_ptr;
        if (tag->type == 0) break;   /* End tag */

        if (tag->type == MB2_TAG_TYPE_MMAP) {
            mb2_mmap_tag_t *mmap = (mb2_mmap_tag_t *)tag;
            uint32_t n = (mmap->size - 16) / mmap->entry_size;

            for (uint32_t i = 0; i < n; i++) {
                mb2_mmap_entry_t *e = &mmap->entries[i];
                uint64_t start_page = e->base_addr / PAGE_SIZE;
                uint64_t page_count = e->length   / PAGE_SIZE;

                if (e->type == MB2_MMAP_TYPE_AVAILABLE) {
                    for (uint64_t p = start_page;
                         p < start_page + page_count && p < PMM_MAX_PAGES; p++) {
                        bitmap_clear(p);
                        pmm_free_phys_pages++;
                        pmm_total_phys_pages++;
                    }
                } else {
                    pmm_total_phys_pages += page_count;
                }
            }
        }

        /* Advance to next tag (aligned to 8 bytes) */
        tag_ptr += ALIGN_UP(tag->size, 8);
    }

    /* Mark low 1 MiB as reserved (BIOS, video memory, etc.) */
    for (uint64_t p = 0; p < 256 && p < PMM_MAX_PAGES; p++) {
        if (!bitmap_test(p)) {
            bitmap_set(p);
            pmm_free_phys_pages--;
        }
    }

    /* Mark the kernel itself as reserved (physical 1 MiB → 4 MiB) */
    pmm_mark_used(KERNEL_PHYS_BASE, 3 * 1024 * 1024);

    pr_info("PMM: %llu MiB total, %llu MiB free\n",
            (pmm_total_phys_pages * PAGE_SIZE) >> 20,
            (pmm_free_phys_pages  * PAGE_SIZE) >> 20);
}

/* ─── Allocation ─────────────────────────────────────────────────────────── */

uintptr_t pmm_alloc_page(void) {
    for (uint64_t i = pmm_last_free_hint; i < PMM_MAX_PAGES; i++) {
        if (!bitmap_test(i)) {
            bitmap_set(i);
            pmm_free_phys_pages--;
            pmm_last_free_hint = i + 1;
            return (uintptr_t)(i * PAGE_SIZE);
        }
    }
    /* Wrap around */
    for (uint64_t i = 0; i < pmm_last_free_hint; i++) {
        if (!bitmap_test(i)) {
            bitmap_set(i);
            pmm_free_phys_pages--;
            pmm_last_free_hint = i + 1;
            return (uintptr_t)(i * PAGE_SIZE);
        }
    }
    kernel_panic("PMM: out of physical memory!");
}

uintptr_t pmm_alloc_pages(size_t count) {
    if (count == 0) return 0;
    if (count == 1) return pmm_alloc_page();

    /* Find a contiguous run of 'count' free pages */
    uint64_t start = 0, run = 0;
    for (uint64_t i = 0; i < PMM_MAX_PAGES; i++) {
        if (!bitmap_test(i)) {
            if (run == 0) start = i;
            run++;
            if (run == count) {
                for (uint64_t j = start; j < start + count; j++) {
                    bitmap_set(j);
                    pmm_free_phys_pages--;
                }
                return (uintptr_t)(start * PAGE_SIZE);
            }
        } else {
            run = 0;
        }
    }
    kernel_panic("PMM: cannot allocate contiguous pages");
}

void pmm_free_page(uintptr_t phys) {
    uint64_t page = phys / PAGE_SIZE;
    if (page >= PMM_MAX_PAGES) return;
    if (!bitmap_test(page)) return;   /* Double-free guard */
    bitmap_clear(page);
    pmm_free_phys_pages++;
    if (page < pmm_last_free_hint) pmm_last_free_hint = page;
}

void pmm_free_pages(uintptr_t phys, size_t count) {
    for (size_t i = 0; i < count; i++)
        pmm_free_page(phys + (uintptr_t)(i * PAGE_SIZE));
}

void pmm_mark_used(uintptr_t phys, size_t size) {
    uint64_t start = phys / PAGE_SIZE;
    uint64_t count = ALIGN_UP(size, PAGE_SIZE) / PAGE_SIZE;
    for (uint64_t i = start; i < start + count && i < PMM_MAX_PAGES; i++) {
        if (!bitmap_test(i)) {
            bitmap_set(i);
            if (pmm_free_phys_pages > 0) pmm_free_phys_pages--;
        }
    }
}

void pmm_mark_free(uintptr_t phys, size_t size) {
    uint64_t start = phys / PAGE_SIZE;
    uint64_t count = size / PAGE_SIZE;
    for (uint64_t i = start; i < start + count && i < PMM_MAX_PAGES; i++) {
        if (bitmap_test(i)) {
            bitmap_clear(i);
            pmm_free_phys_pages++;
        }
    }
}

void pmm_get_stats(pmm_stats_t *stats) {
    stats->total_pages    = pmm_total_phys_pages;
    stats->free_pages     = pmm_free_phys_pages;
    stats->used_pages     = pmm_total_phys_pages - pmm_free_phys_pages;
    stats->reserved_pages = 0;   /* Counted as "used" */
}

uint64_t pmm_total_memory(void) {
    return pmm_total_phys_pages * PAGE_SIZE;
}
