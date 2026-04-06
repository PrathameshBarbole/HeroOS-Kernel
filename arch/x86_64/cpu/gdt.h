#ifndef ARCH_GDT_H
#define ARCH_GDT_H

#include <kernel/types.h>

/* GDT segment selectors */
#define GDT_NULL_SEG     0x00
#define GDT_KERNEL_CODE  0x08
#define GDT_KERNEL_DATA  0x10
#define GDT_USER_CODE    0x18
#define GDT_USER_DATA    0x20
#define GDT_TSS          0x28

#define GDT_ENTRY_COUNT  7   /* null, kcode, kdata, ucode, udata, tss-lo, tss-hi */

/* Access byte flags */
#define GDT_ACCESS_PRESENT     0x80
#define GDT_ACCESS_RING0       0x00
#define GDT_ACCESS_RING3       0x60
#define GDT_ACCESS_SYSTEM      0x10
#define GDT_ACCESS_EXEC        0x08
#define GDT_ACCESS_DC          0x04
#define GDT_ACCESS_RW          0x02
#define GDT_ACCESS_ACCESSED    0x01

/* Granularity/flags byte */
#define GDT_FLAG_GRANULARITY   0x80  /* Page granularity */
#define GDT_FLAG_SIZE          0x40  /* 32-bit protected mode */
#define GDT_FLAG_LONG          0x20  /* 64-bit long mode */

/* GDT entry (8 bytes) */
typedef struct PACKED {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  access;
    uint8_t  limit_high_flags;
    uint8_t  base_high;
} gdt_entry_t;

/* GDT pointer (GDTR register value) */
typedef struct PACKED {
    uint16_t  limit;
    uint64_t  base;
} gdt_ptr_t;

/* Task State Segment (64-bit) */
typedef struct PACKED {
    uint32_t reserved0;
    uint64_t rsp[3];       /* Ring 0-2 stack pointers */
    uint64_t reserved1;
    uint64_t ist[7];       /* Interrupt Stack Table */
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iomap_base;
} tss_t;

/* 64-bit system segment descriptor (16 bytes for TSS) */
typedef struct PACKED {
    uint16_t length;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  flags1;
    uint8_t  flags2;
    uint8_t  base_high;
    uint32_t base_upper;
    uint32_t reserved;
} tss_descriptor_t;

void gdt_init(void);
void gdt_set_kernel_stack(uintptr_t stack_top);

/* Defined in gdt.asm — flushes GDT and reloads segment registers */
extern void gdt_flush(uint64_t gdt_ptr);
extern void tss_flush(void);

#endif /* ARCH_GDT_H */
