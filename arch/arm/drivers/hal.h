/*
 * ARM/AArch64 HAL stub — Raspberry Pi 4/5 BSP placeholder
 *
 * This file stubs out the AArch64 platform so the build system can
 * reference ARM targets without actual hardware-specific code yet.
 * Full implementation (UART, GIC, timer, GPIO) is a Phase 3 milestone.
 */

#ifndef ARM_HAL_H
#define ARM_HAL_H

#include <kernel/types.h>

/* Raspberry Pi 4 peripheral base */
#define RPI4_PERIPHERAL_BASE  0xFE000000UL
#define RPI4_UART0_BASE       (RPI4_PERIPHERAL_BASE + 0x201000)
#define RPI4_SYSTIMER_BASE    (RPI4_PERIPHERAL_BASE + 0x003000)
#define RPI4_GPIO_BASE        (RPI4_PERIPHERAL_BASE + 0x200000)
#define RPI4_I2C0_BASE        (RPI4_PERIPHERAL_BASE + 0x205000)
#define RPI4_SPI0_BASE        (RPI4_PERIPHERAL_BASE + 0x204000)

/* Raspberry Pi 5 uses RP1 chip at a different base */
#define RPI5_RP1_BASE         0x1F00000000ULL

/* AArch64 MMIO helpers */
static ALWAYS_INLINE void arm_mmio_write32(uintptr_t addr, uint32_t val) {
    *(volatile uint32_t *)addr = val;
    __asm__ volatile("dsb sy" ::: "memory");
}

static ALWAYS_INLINE uint32_t arm_mmio_read32(uintptr_t addr) {
    __asm__ volatile("dsb sy" ::: "memory");
    return *(volatile uint32_t *)addr;
}

/* ARM64 system register access */
#define READ_SYSREG(reg)  ({ uint64_t _v; __asm__ volatile("mrs %0, " #reg : "=r"(_v)); _v; })
#define WRITE_SYSREG(reg, val) __asm__ volatile("msr " #reg ", %0" :: "r"((uint64_t)(val)))

/* CPU core ID */
static ALWAYS_INLINE uint64_t arm_get_core_id(void) {
    uint64_t mpidr = READ_SYSREG(mpidr_el1);
    return mpidr & 0xFF;
}

/* AArch64 interrupt control */
static ALWAYS_INLINE void arm_enable_irq(void)  { __asm__ volatile("msr daifclr, #2"); }
static ALWAYS_INLINE void arm_disable_irq(void) { __asm__ volatile("msr daifset, #2"); }
static ALWAYS_INLINE void arm_wfi(void)          { __asm__ volatile("wfi"); }
static ALWAYS_INLINE void arm_wfe(void)          { __asm__ volatile("wfe"); }
static ALWAYS_INLINE void arm_isb(void)          { __asm__ volatile("isb"); }
static ALWAYS_INLINE void arm_dsb_sy(void)       { __asm__ volatile("dsb sy" ::: "memory"); }

/* GIC-400 (used in RPi 4) */
#define GIC_DIST_BASE    0xFF841000UL
#define GIC_CPU_BASE     0xFF842000UL

/* Device Tree (DTB) */
typedef struct {
    uint32_t magic;       /* 0xD00DFEED */
    uint32_t totalsize;
    uint32_t off_dt_struct;
    uint32_t off_dt_strings;
    uint32_t off_mem_rsvmap;
    uint32_t version;
    uint32_t last_comp_version;
    uint32_t boot_cpuid_phys;
    uint32_t size_dt_strings;
    uint32_t size_dt_struct;
} fdt_header_t;

#define FDT_MAGIC  0xD00DFEEDU

int  dtb_init(uintptr_t dtb_phys);
int  dtb_find_property(const char *node_path, const char *prop_name,
                        void *buf, size_t bufsz);

/* BSP init functions */
void rpi4_early_init(void);
void rpi4_uart_init(void);
void rpi4_uart_putc(char c);

#endif /* ARM_HAL_H */
