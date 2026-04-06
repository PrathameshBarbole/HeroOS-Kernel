#ifndef ARCH_PAGING_H
#define ARCH_PAGING_H

#include <kernel/types.h>

/* Page table entry flags */
#define PTE_PRESENT     BIT(0)
#define PTE_WRITABLE    BIT(1)
#define PTE_USER        BIT(2)
#define PTE_WRITE_THRU  BIT(3)
#define PTE_CACHE_DIS   BIT(4)
#define PTE_ACCESSED    BIT(5)
#define PTE_DIRTY       BIT(6)
#define PTE_HUGE        BIT(7)   /* PD entry: 2MiB page */
#define PTE_GLOBAL      BIT(8)
#define PTE_NO_EXEC     BIT(63)  /* NX bit */

/* Convenience flag combinations */
#define PTE_KERNEL_RO   (PTE_PRESENT | PTE_GLOBAL)
#define PTE_KERNEL_RW   (PTE_PRESENT | PTE_WRITABLE | PTE_GLOBAL)
#define PTE_USER_RO     (PTE_PRESENT | PTE_USER)
#define PTE_USER_RW     (PTE_PRESENT | PTE_WRITABLE | PTE_USER)

/* Page table index extraction from virtual address */
#define PML4_IDX(va)    (((va) >> 39) & 0x1FF)
#define PDPT_IDX(va)    (((va) >> 30) & 0x1FF)
#define PD_IDX(va)      (((va) >> 21) & 0x1FF)
#define PT_IDX(va)      (((va) >> 12) & 0x1FF)
#define PAGE_OFFSET(va) ((va) & 0xFFF)

/* Number of entries per page table level */
#define PT_ENTRIES      512

typedef uint64_t pte_t;
typedef pte_t    pt_t[PT_ENTRIES];   /* One page table (4 KiB) */

/* Extract physical address from a PTE */
#define PTE_ADDR(pte)   ((pte) & 0x000FFFFFFFFFF000ULL)
#define PTE_FLAGS(pte)  ((pte) & ~0x000FFFFFFFFFF000ULL)

void paging_init(void);
void paging_map(uintptr_t virt, uintptr_t phys, uint64_t flags);
void paging_unmap(uintptr_t virt);
uintptr_t paging_virt_to_phys(uintptr_t virt);

/* Switch to a new PML4 (cr3 load) */
void paging_switch_directory(uintptr_t pml4_phys);

/* Invalidate a single TLB entry */
static ALWAYS_INLINE void tlb_flush_page(uintptr_t virt) {
    __asm__ volatile("invlpg (%0)" : : "r"(virt) : "memory");
}

/* Full TLB flush (reload CR3) */
static ALWAYS_INLINE void tlb_flush_all(void) {
    uint64_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    __asm__ volatile("mov %0, %%cr3" : : "r"(cr3) : "memory");
}

#endif /* ARCH_PAGING_H */
