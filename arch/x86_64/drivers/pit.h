#ifndef ARCH_PIT_H
#define ARCH_PIT_H

#include <kernel/types.h>

/* PIT I/O ports */
#define PIT_CHANNEL0    0x40
#define PIT_CHANNEL1    0x41
#define PIT_CHANNEL2    0x42
#define PIT_COMMAND     0x43

/* PIT command byte fields */
#define PIT_CMD_CHANNEL0    0x00
#define PIT_CMD_LOBYTE_HI   0x30  /* Access: lo/hi byte */
#define PIT_CMD_MODE2       0x04  /* Rate generator */
#define PIT_CMD_BINARY      0x00  /* Binary mode */

#define PIT_BASE_FREQ   1193180UL  /* Hz */
#define PIT_TARGET_HZ   1000       /* Desired tick rate */
#define PIT_DIVISOR     (PIT_BASE_FREQ / PIT_TARGET_HZ)

void pit_init(uint32_t frequency_hz);
uint64_t pit_get_ticks(void);
void pit_sleep_ms(uint32_t ms);

/* RTC (Real-Time Clock) */
#define RTC_ADDRESS_PORT  0x70
#define RTC_DATA_PORT     0x71

typedef struct {
    uint8_t  seconds;
    uint8_t  minutes;
    uint8_t  hours;
    uint8_t  day;
    uint8_t  month;
    uint16_t year;
    uint8_t  weekday;
} rtc_time_t;

void     rtc_init(void);
void     rtc_read(rtc_time_t *t);
uint64_t rtc_timestamp(void);  /* Seconds since epoch (approx.) */

#endif /* ARCH_PIT_H */
