#ifndef KERNEL_HAL_H
#define KERNEL_HAL_H

#include <kernel/types.h>

/*
 * Hardware Abstraction Layer (HAL)
 *
 * All architecture-specific operations are accessed through this interface.
 * Platform ports (x86_64, ARM) implement these functions in arch/*/hal.c.
 */

/* ── CPU control ──────────────────────────────────────── */
void hal_cpu_halt(void);                /* Halt CPU until next interrupt */
void hal_cpu_disable_interrupts(void);
void hal_cpu_enable_interrupts(void);
bool hal_cpu_interrupts_enabled(void);
void hal_cpu_idle(void);                /* Low-power wait */

/* ── I/O port access (x86 only; no-op on ARM) ─────────── */
void    hal_outb(uint16_t port, uint8_t  val);
void    hal_outw(uint16_t port, uint16_t val);
void    hal_outl(uint16_t port, uint32_t val);
uint8_t  hal_inb(uint16_t port);
uint16_t hal_inw(uint16_t port);
uint32_t hal_inl(uint16_t port);
void    hal_io_wait(void);

/* ── Memory-mapped I/O ────────────────────────────────── */
static ALWAYS_INLINE uint8_t  mmio_read8 (uintptr_t addr){ return *(volatile uint8_t  *)addr; }
static ALWAYS_INLINE uint16_t mmio_read16(uintptr_t addr){ return *(volatile uint16_t *)addr; }
static ALWAYS_INLINE uint32_t mmio_read32(uintptr_t addr){ return *(volatile uint32_t *)addr; }
static ALWAYS_INLINE uint64_t mmio_read64(uintptr_t addr){ return *(volatile uint64_t *)addr; }
static ALWAYS_INLINE void mmio_write8 (uintptr_t addr, uint8_t  v){ *(volatile uint8_t  *)addr = v; }
static ALWAYS_INLINE void mmio_write16(uintptr_t addr, uint16_t v){ *(volatile uint16_t *)addr = v; }
static ALWAYS_INLINE void mmio_write32(uintptr_t addr, uint32_t v){ *(volatile uint32_t *)addr = v; }
static ALWAYS_INLINE void mmio_write64(uintptr_t addr, uint64_t v){ *(volatile uint64_t *)addr = v; }

/* ── System timer ──────────────────────────────────────── */
void     hal_timer_init(uint32_t frequency_hz);
uint64_t hal_timer_ticks(void);
void     hal_timer_sleep_ms(uint32_t ms);

/* ── Serial debug output ───────────────────────────────── */
void hal_serial_init(void);
void hal_serial_putc(char c);
void hal_serial_puts(const char *s);

/* ── Platform identification ──────────────────────────── */
typedef enum {
    PLATFORM_X86_64 = 0,
    PLATFORM_AARCH64,
    PLATFORM_RISCV64,
    PLATFORM_UNKNOWN,
} platform_id_t;

platform_id_t hal_get_platform(void);
const char   *hal_get_platform_name(void);

/* ── Early initialization (called from boot) ──────────── */
void hal_early_init(void);   /* Before MM is up */
void hal_late_init(void);    /* After MM + scheduler are up */

#endif /* KERNEL_HAL_H */
