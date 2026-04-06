#include "serial.h"
#include <kernel/types.h>

/* ─── Port I/O helpers ───────────────────────────────────────────────────── */

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t val;
    __asm__ volatile("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

/* ─── Serial UART (16550A compatible) ──────────────────────────────────── */

void serial_init(uint16_t port, uint16_t baud_divisor) {
    outb(port + 1, 0x00);   /* Disable all interrupts */
    outb(port + 3, 0x80);   /* Enable DLAB to set baud rate divisor */
    outb(port + 0, (uint8_t)(baud_divisor & 0xFF));   /* Divisor low byte */
    outb(port + 1, (uint8_t)(baud_divisor >> 8));     /* Divisor high byte */
    outb(port + 3, 0x03);   /* 8 bits, no parity, one stop bit (DLAB off) */
    outb(port + 2, 0xC7);   /* Enable FIFO, clear, with 14-byte threshold */
    outb(port + 4, 0x0B);   /* IRQs enabled, RTS/DSR set */
    outb(port + 4, 0x0F);   /* Set in normal operation mode */
}

bool serial_received(uint16_t port) {
    return (inb(port + 5) & 0x01) != 0;
}

bool serial_transmit_empty(uint16_t port) {
    return (inb(port + 5) & 0x20) != 0;
}

void serial_putc(uint16_t port, char c) {
    /* Busy-wait until transmit buffer is empty */
    while (!serial_transmit_empty(port));
    if (c == '\n') {
        outb(port, '\r');
        while (!serial_transmit_empty(port));
    }
    outb(port, (uint8_t)c);
}

void serial_puts(uint16_t port, const char *s) {
    while (*s) serial_putc(port, *s++);
}

char serial_getc(uint16_t port) {
    while (!serial_received(port));
    return (char)inb(port);
}

/* ─── Default early-boot serial (COM1 at 115200 baud) ─────────────────── */

void serial_early_init(void) {
    serial_init(SERIAL_COM1, SERIAL_BAUD_115200);
}

/* Called by printk */
void serial_write_char(char c) {
    serial_putc(SERIAL_COM1, c);
}
