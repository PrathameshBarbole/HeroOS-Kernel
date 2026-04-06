#include "gdt.h"
#include <lib/string.h>

/* ─── GDT and TSS storage ────────────────────────────────────────────────── */

static gdt_entry_t gdt[GDT_ENTRY_COUNT];
static gdt_ptr_t   gdt_ptr;
static tss_t       kernel_tss;

/* ─── Helpers ────────────────────────────────────────────────────────────── */

static void gdt_set_entry(int index, uint32_t base, uint32_t limit,
                           uint8_t access, uint8_t flags) {
    gdt[index].base_low        = (uint16_t)(base & 0xFFFF);
    gdt[index].base_mid        = (uint8_t)((base >> 16) & 0xFF);
    gdt[index].base_high       = (uint8_t)((base >> 24) & 0xFF);
    gdt[index].limit_low       = (uint16_t)(limit & 0xFFFF);
    gdt[index].limit_high_flags = (uint8_t)(((limit >> 16) & 0x0F) | (flags & 0xF0));
    gdt[index].access          = access;
}

/* Install a 64-bit system descriptor (TSS — takes two GDT slots) */
static void gdt_set_tss(int index, uint64_t base, uint32_t limit) {
    /* First 8 bytes — same layout as a regular descriptor */
    gdt[index].limit_low       = (uint16_t)(limit & 0xFFFF);
    gdt[index].base_low        = (uint16_t)(base & 0xFFFF);
    gdt[index].base_mid        = (uint8_t)((base >> 16) & 0xFF);
    gdt[index].access          = 0x89;  /* Present, DPL=0, Type=9 (TSS available) */
    gdt[index].limit_high_flags = (uint8_t)(((limit >> 16) & 0x0F));
    gdt[index].base_high       = (uint8_t)((base >> 24) & 0xFF);

    /* Second slot — upper 32 bits of 64-bit base + reserved */
    tss_descriptor_t *tss_hi = (tss_descriptor_t *)&gdt[index];
    (void)tss_hi;
    /* Write the upper 32 bits into the next descriptor slot */
    uint32_t *hi_slot = (uint32_t *)&gdt[index + 1];
    hi_slot[0] = (uint32_t)(base >> 32);
    hi_slot[1] = 0;
}

/* ─── GDT initialisation ─────────────────────────────────────────────────── */

void gdt_init(void) {
    /* 0x00: Null descriptor */
    gdt_set_entry(0, 0, 0, 0, 0);

    /* 0x08: 64-bit kernel code  (L=1, DPL=0, Execute/Read) */
    gdt_set_entry(1,
        0, 0xFFFFF,
        GDT_ACCESS_PRESENT | GDT_ACCESS_SYSTEM | GDT_ACCESS_EXEC | GDT_ACCESS_RW,
        GDT_FLAG_GRANULARITY | GDT_FLAG_LONG);

    /* 0x10: 64-bit kernel data  (DPL=0, Read/Write) */
    gdt_set_entry(2,
        0, 0xFFFFF,
        GDT_ACCESS_PRESENT | GDT_ACCESS_SYSTEM | GDT_ACCESS_RW,
        GDT_FLAG_GRANULARITY | GDT_FLAG_SIZE);

    /* 0x18: 64-bit user code    (L=1, DPL=3) */
    gdt_set_entry(3,
        0, 0xFFFFF,
        GDT_ACCESS_PRESENT | GDT_ACCESS_RING3 | GDT_ACCESS_SYSTEM |
            GDT_ACCESS_EXEC | GDT_ACCESS_RW,
        GDT_FLAG_GRANULARITY | GDT_FLAG_LONG);

    /* 0x20: 64-bit user data    (DPL=3) */
    gdt_set_entry(4,
        0, 0xFFFFF,
        GDT_ACCESS_PRESENT | GDT_ACCESS_RING3 | GDT_ACCESS_SYSTEM | GDT_ACCESS_RW,
        GDT_FLAG_GRANULARITY | GDT_FLAG_SIZE);

    /* 0x28: TSS (two slots — 0x28 and 0x30) */
    memset(&kernel_tss, 0, sizeof(kernel_tss));
    kernel_tss.iomap_base = sizeof(tss_t);
    gdt_set_tss(5, (uint64_t)(uintptr_t)&kernel_tss, sizeof(tss_t) - 1);

    /* Load the GDT and flush segment registers */
    gdt_ptr.limit = (uint16_t)(sizeof(gdt) - 1);
    gdt_ptr.base  = (uint64_t)(uintptr_t)gdt;
    gdt_flush((uint64_t)(uintptr_t)&gdt_ptr);
    tss_flush();
}

void gdt_set_kernel_stack(uintptr_t stack_top) {
    kernel_tss.rsp[0] = (uint64_t)stack_top;
}
