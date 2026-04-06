#include <kernel/mm/vmm.h>
#include <kernel/mm/pmm.h>
#include <arch/x86_64/mm/paging.h>
#include <kernel/printk.h>
#include <lib/string.h>
#include <kernel/mm/kheap.h>

addr_space_t *kernel_addr_space = NULL;

/* ─── Internal helper: get or create a page-table level ─────────────────── */

static pte_t *get_or_create_table(pte_t *parent, uint16_t idx, uint64_t flags) {
    if (parent[idx] & PTE_PRESENT) {
        return (pte_t *)(uintptr_t)PTE_ADDR(parent[idx]);
    }
    uintptr_t phys = pmm_alloc_page();
    memset((void *)phys, 0, PAGE_SIZE);
    parent[idx] = (pte_t)(phys | flags);
    return (pte_t *)(uintptr_t)phys;
}

/* ─── Map one page ───────────────────────────────────────────────────────── */

void paging_map(uintptr_t virt, uintptr_t phys, uint64_t flags) {
    uint16_t pml4_i = (uint16_t)PML4_IDX(virt);
    uint16_t pdpt_i = (uint16_t)PDPT_IDX(virt);
    uint16_t pd_i   = (uint16_t)PD_IDX(virt);
    uint16_t pt_i   = (uint16_t)PT_IDX(virt);

    /* We need the PML4 physical address — get from CR3 */
    uint64_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    pte_t *pml4 = (pte_t *)(uintptr_t)(cr3 & ~0xFFFULL);

    pte_t *pdpt = get_or_create_table(pml4, pml4_i, PTE_PRESENT | PTE_WRITABLE);
    pte_t *pd   = get_or_create_table(pdpt, pdpt_i, PTE_PRESENT | PTE_WRITABLE);
    pte_t *pt   = get_or_create_table(pd,   pd_i,   PTE_PRESENT | PTE_WRITABLE);

    pt[pt_i] = (pte_t)(phys | flags);
    tlb_flush_page(virt);
}

void paging_unmap(uintptr_t virt) {
    uint64_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    pte_t *pml4 = (pte_t *)(uintptr_t)(cr3 & ~0xFFFULL);

    uint16_t pml4_i = (uint16_t)PML4_IDX(virt);
    if (!(pml4[pml4_i] & PTE_PRESENT)) return;
    pte_t *pdpt = (pte_t *)(uintptr_t)PTE_ADDR(pml4[pml4_i]);

    uint16_t pdpt_i = (uint16_t)PDPT_IDX(virt);
    if (!(pdpt[pdpt_i] & PTE_PRESENT)) return;
    pte_t *pd = (pte_t *)(uintptr_t)PTE_ADDR(pdpt[pdpt_i]);

    uint16_t pd_i = (uint16_t)PD_IDX(virt);
    if (!(pd[pd_i] & PTE_PRESENT)) return;
    pte_t *pt = (pte_t *)(uintptr_t)PTE_ADDR(pd[pd_i]);

    uint16_t pt_i = (uint16_t)PT_IDX(virt);
    pt[pt_i] = 0;
    tlb_flush_page(virt);
}

uintptr_t paging_virt_to_phys(uintptr_t virt) {
    uint64_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    pte_t *pml4 = (pte_t *)(uintptr_t)(cr3 & ~0xFFFULL);

    uint16_t pml4_i = (uint16_t)PML4_IDX(virt);
    if (!(pml4[pml4_i] & PTE_PRESENT)) return 0;
    pte_t *pdpt = (pte_t *)(uintptr_t)PTE_ADDR(pml4[pml4_i]);

    uint16_t pdpt_i = (uint16_t)PDPT_IDX(virt);
    if (!(pdpt[pdpt_i] & PTE_PRESENT)) return 0;
    pte_t *pd = (pte_t *)(uintptr_t)PTE_ADDR(pdpt[pdpt_i]);

    uint16_t pd_i = (uint16_t)PD_IDX(virt);
    if (!(pd[pd_i] & PTE_PRESENT)) return 0;
    if (pd[pd_i] & PTE_HUGE)
        return (uintptr_t)PTE_ADDR(pd[pd_i]) + (virt & 0x1FFFFF);
    pte_t *pt = (pte_t *)(uintptr_t)PTE_ADDR(pd[pd_i]);

    uint16_t pt_i = (uint16_t)PT_IDX(virt);
    if (!(pt[pt_i] & PTE_PRESENT)) return 0;
    return (uintptr_t)PTE_ADDR(pt[pt_i]) + PAGE_OFFSET(virt);
}

void paging_switch_directory(uintptr_t pml4_phys) {
    __asm__ volatile("mov %0, %%cr3" : : "r"(pml4_phys) : "memory");
}

void paging_init(void) {
    pr_info("Paging: 4-level paging active (PML4 already set up by boot.asm)\n");
}

/* ─── VMM ────────────────────────────────────────────────────────────────── */

void vmm_init(void) {
    paging_init();
    pr_info("VMM initialised\n");
}

addr_space_t *vmm_create_address_space(void) {
    addr_space_t *as = (addr_space_t *)kmalloc(sizeof(addr_space_t));
    if (!as) return NULL;

    uintptr_t pml4_phys = pmm_alloc_page();
    memset((void *)pml4_phys, 0, PAGE_SIZE);

    as->pml4_phys  = pml4_phys;
    as->heap_start = 0x0000000000400000ULL;  /* User heap starts at 4 MiB */
    as->heap_end   = as->heap_start;
    as->stack_top  = 0x00007FFFFFFFE000ULL;  /* User stack near top of user space */

    return as;
}

void vmm_destroy_address_space(addr_space_t *as) {
    if (!as) return;
    pmm_free_page(as->pml4_phys);
    kfree(as);
}

void vmm_switch_address_space(addr_space_t *as) {
    if (as) paging_switch_directory(as->pml4_phys);
}

int vmm_map_range(addr_space_t *as, uintptr_t virt, uintptr_t phys,
                  size_t size, uint64_t flags) {
    (void)as;  /* TODO: use as->pml4_phys when multi-process support is active */
    size = ALIGN_UP(size, PAGE_SIZE);
    for (size_t offset = 0; offset < size; offset += PAGE_SIZE)
        paging_map(virt + offset, phys + offset, flags);
    return 0;
}

int vmm_unmap_range(addr_space_t *as, uintptr_t virt, size_t size) {
    (void)as;
    size = ALIGN_UP(size, PAGE_SIZE);
    for (size_t offset = 0; offset < size; offset += PAGE_SIZE)
        paging_unmap(virt + offset);
    return 0;
}

uintptr_t vmm_alloc_pages(addr_space_t *as, size_t count, uint64_t flags) {
    uintptr_t phys = pmm_alloc_pages(count);
    uintptr_t virt = as ? as->heap_end : phys;

    if (as) {
        vmm_map_range(as, virt, phys, count * PAGE_SIZE, flags);
        as->heap_end += count * PAGE_SIZE;
    }
    return virt;
}

uintptr_t vmm_virt_to_phys(addr_space_t *as, uintptr_t virt) {
    (void)as;
    return paging_virt_to_phys(virt);
}
