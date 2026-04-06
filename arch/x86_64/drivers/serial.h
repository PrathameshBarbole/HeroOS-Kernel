#ifndef ARCH_SERIAL_H
#define ARCH_SERIAL_H

#include <kernel/types.h>

/* COM port base addresses */
#define SERIAL_COM1  0x3F8
#define SERIAL_COM2  0x2F8
#define SERIAL_COM3  0x3E8
#define SERIAL_COM4  0x2E8

/* Baud rates (divisors for 115200 base) */
#define SERIAL_BAUD_115200  1
#define SERIAL_BAUD_57600   2
#define SERIAL_BAUD_38400   3
#define SERIAL_BAUD_9600    12

void serial_init(uint16_t port, uint16_t baud_divisor);
void serial_putc(uint16_t port, char c);
void serial_puts(uint16_t port, const char *s);
char serial_getc(uint16_t port);
bool serial_received(uint16_t port);
bool serial_transmit_empty(uint16_t port);

/* Default serial output (COM1) used by printk */
void serial_early_init(void);
void serial_write_char(char c);

#endif /* ARCH_SERIAL_H */
