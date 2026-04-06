#ifndef KERNEL_VMM_H
#define KERNEL_VMM_H

#include <kernel/types.h>

/* VMM region types */
#define VMR_KERNEL_CODE  0x01
#define VMR_KERNEL_DATA  0x02
#define VMR_KERNEL_HEAP  0x03
#define VMR_USER_CODE    0x10
#define VMR_USER_DATA    0x11
#define VMR_USER_HEAP    0x12
#define VMR_USER_STACK   0x13
#define VMR_MMIO         0x20

/* Address space (per-process PML4) */
typedef struct {
    uintptr_t pml4_phys;   /* Physical address of PML4 */
    uintptr_t heap_start;
    uintptr_t heap_end;
    uintptr_t stack_top;
} addr_space_t;

/* VMM public API */
void      vmm_init(void);
addr_space_t *vmm_create_address_space(void);
void      vmm_destroy_address_space(addr_space_t *as);
void      vmm_switch_address_space(addr_space_t *as);

/* Map a virtual range (all pages) */
int       vmm_map_range(addr_space_t *as, uintptr_t virt, uintptr_t phys,
                        size_t size, uint64_t flags);
int       vmm_unmap_range(addr_space_t *as, uintptr_t virt, size_t size);

/* Allocate and map virtual pages (physical memory allocated internally) */
uintptr_t vmm_alloc_pages(addr_space_t *as, size_t count, uint64_t flags);

/* Translate virtual to physical in the given address space */
uintptr_t vmm_virt_to_phys(addr_space_t *as, uintptr_t virt);

/* Kernel address space (global) */
extern addr_space_t *kernel_addr_space;

#endif /* KERNEL_VMM_H */
