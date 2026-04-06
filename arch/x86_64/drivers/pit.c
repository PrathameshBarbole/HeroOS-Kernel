#include "pit.h"
#include <arch/x86_64/cpu/idt.h>
#include <kernel/printk.h>

/* ─── Port I/O ───────────────────────────────────────────────────────────── */

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}
static inline uint8_t inb(uint16_t port) {
    uint8_t v; __asm__ volatile("inb %1, %0" : "=a"(v) : "Nd"(port)); return v;
}

/* ─── PIT ────────────────────────────────────────────────────────────────── */

static volatile uint64_t pit_ticks = 0;

static void pit_irq_handler(interrupt_frame_t *frame) {
    (void)frame;
    pit_ticks++;

    /* Notify scheduler */
    extern void sched_tick(void);
    sched_tick();
}

void pit_init(uint32_t frequency_hz) {
    if (frequency_hz == 0) frequency_hz = PIT_TARGET_HZ;
    uint32_t divisor = (uint32_t)(PIT_BASE_FREQ / frequency_hz);

    /* Command: channel 0, lo/hi byte, mode 2 (rate generator), binary */
    outb(PIT_COMMAND, PIT_CMD_CHANNEL0 | PIT_CMD_LOBYTE_HI | PIT_CMD_MODE2 | PIT_CMD_BINARY);
    outb(PIT_CHANNEL0, (uint8_t)(divisor & 0xFF));
    outb(PIT_CHANNEL0, (uint8_t)((divisor >> 8) & 0xFF));

    irq_register_handler(0, pit_irq_handler);

    pr_info("PIT initialised: %u Hz (divisor %u)\n", frequency_hz, divisor);
}

uint64_t pit_get_ticks(void) {
    return pit_ticks;
}

void pit_sleep_ms(uint32_t ms) {
    uint64_t target = pit_ticks + (uint64_t)ms;
    while (pit_ticks < target)
        __asm__ volatile("hlt");
}

/* ─── RTC ────────────────────────────────────────────────────────────────── */

static uint8_t rtc_read_reg(uint8_t reg) {
    outb(RTC_ADDRESS_PORT, reg);
    return inb(RTC_DATA_PORT);
}

static uint8_t bcd_to_bin(uint8_t bcd) {
    return (uint8_t)(((bcd >> 4) * 10) + (bcd & 0x0F));
}

static bool rtc_updating(void) {
    outb(RTC_ADDRESS_PORT, 0x0A);
    return (inb(RTC_DATA_PORT) & 0x80) != 0;
}

void rtc_init(void) {
    /* Nothing special to init — CMOS is always present */
    pr_info("RTC initialised\n");
}

void rtc_read(rtc_time_t *t) {
    /* Wait for RTC to finish any update cycle */
    while (rtc_updating());

    uint8_t status_b = rtc_read_reg(0x0B);
    bool binary_mode = (status_b & 0x04) != 0;
    bool hour24      = (status_b & 0x02) != 0;

    t->seconds = rtc_read_reg(0x00);
    t->minutes = rtc_read_reg(0x02);
    t->hours   = rtc_read_reg(0x04);
    t->weekday = rtc_read_reg(0x06);
    t->day     = rtc_read_reg(0x07);
    t->month   = rtc_read_reg(0x08);
    uint8_t year_raw = rtc_read_reg(0x09);
    uint8_t century  = rtc_read_reg(0x32);

    if (!binary_mode) {
        t->seconds = bcd_to_bin(t->seconds);
        t->minutes = bcd_to_bin(t->minutes);
        t->hours   = bcd_to_bin(t->hours & 0x7F);
        t->weekday = bcd_to_bin(t->weekday);
        t->day     = bcd_to_bin(t->day);
        t->month   = bcd_to_bin(t->month);
        year_raw   = bcd_to_bin(year_raw);
        century    = bcd_to_bin(century);
    }

    /* Handle 12-hour mode */
    if (!hour24 && (t->hours & 0x80)) {
        t->hours = (uint8_t)((t->hours & 0x7F) + 12);
    }

    t->year = (uint16_t)((century ? century * 100 : 2000) + year_raw);
}

uint64_t rtc_timestamp(void) {
    rtc_time_t t;
    rtc_read(&t);

    /* Simplified Unix timestamp calculation */
    static const uint16_t days_before_month[12] =
        { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };

    uint32_t y = t.year - 1970;
    uint32_t leap_days = (y / 4) - (y / 100) + (y / 400);
    uint32_t days = y * 365 + leap_days;
    if (t.month > 0 && t.month <= 12)
        days += days_before_month[t.month - 1];
    /* Leap year correction for current year */
    bool is_leap = (t.year % 4 == 0 && (t.year % 100 != 0 || t.year % 400 == 0));
    if (is_leap && t.month > 2) days++;
    days += t.day - 1;

    return (uint64_t)days * 86400ULL
         + (uint64_t)t.hours   * 3600ULL
         + (uint64_t)t.minutes * 60ULL
         + (uint64_t)t.seconds;
}
