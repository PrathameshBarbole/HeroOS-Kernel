#include <kernel/printk.h>
#include <lib/string.h>
#include <kernel/types.h>

/* Forward declaration — implemented in arch serial driver */
extern void serial_write_char(char c);

/* ─── Formatted output ───────────────────────────────────────────────────── */

static void print_str(const char *s) {
    if (!s) s = "(null)";
    while (*s) serial_write_char(*s++);
}

static void print_uint(uint64_t val, int base, int width, char pad, bool upper) {
    char buf[65];
    const char *hex_lo = "0123456789abcdef";
    const char *hex_up = "0123456789ABCDEF";
    const char *hex = upper ? hex_up : hex_lo;

    if (base < 2 || base > 16) { serial_write_char('?'); return; }

    char tmp[65];
    int i = 0;
    if (val == 0) { tmp[i++] = '0'; }
    while (val) { tmp[i++] = hex[val % (uint64_t)base]; val /= (uint64_t)base; }

    /* Padding */
    int len = i;
    while (width > len) { serial_write_char(pad); width--; }

    while (i--) serial_write_char(tmp[i]);
}

static void print_int(int64_t val, int width, char pad) {
    if (val < 0) {
        serial_write_char('-');
        if (width > 0) width--;
        print_uint((uint64_t)(-val), 10, width, pad, false);
    } else {
        print_uint((uint64_t)val, 10, width, pad, false);
    }
}

void vprintk(const char *fmt, __builtin_va_list ap) {
    for (; *fmt; fmt++) {
        if (*fmt != '%') { serial_write_char(*fmt); continue; }

        fmt++;  /* skip '%' */

        /* Parse flags */
        char pad = ' ';
        if (*fmt == '0') { pad = '0'; fmt++; }

        /* Parse width */
        int width = 0;
        while (*fmt >= '0' && *fmt <= '9') {
            width = width * 10 + (*fmt - '0');
            fmt++;
        }

        /* Parse length modifier */
        bool is_long = false, is_longlong = false;
        if (*fmt == 'l') {
            is_long = true; fmt++;
            if (*fmt == 'l') { is_longlong = true; fmt++; }
        } else if (*fmt == 'z') {
            is_long = true; fmt++;
        }

        switch (*fmt) {
        case 'd': case 'i': {
            int64_t v = is_longlong ? __builtin_va_arg(ap, long long)
                      : is_long    ? __builtin_va_arg(ap, long)
                                   : __builtin_va_arg(ap, int);
            print_int(v, width, pad);
            break;
        }
        case 'u': {
            uint64_t v = is_longlong ? __builtin_va_arg(ap, unsigned long long)
                       : is_long    ? __builtin_va_arg(ap, unsigned long)
                                    : __builtin_va_arg(ap, unsigned int);
            print_uint(v, 10, width, pad, false);
            break;
        }
        case 'x': {
            uint64_t v = is_longlong ? __builtin_va_arg(ap, unsigned long long)
                       : is_long    ? __builtin_va_arg(ap, unsigned long)
                                    : __builtin_va_arg(ap, unsigned int);
            print_uint(v, 16, width, pad, false);
            break;
        }
        case 'X': {
            uint64_t v = is_longlong ? __builtin_va_arg(ap, unsigned long long)
                       : is_long    ? __builtin_va_arg(ap, unsigned long)
                                    : __builtin_va_arg(ap, unsigned int);
            print_uint(v, 16, width, pad, true);
            break;
        }
        case 'o': {
            uint64_t v = __builtin_va_arg(ap, unsigned int);
            print_uint(v, 8, width, pad, false);
            break;
        }
        case 'b': {
            uint64_t v = __builtin_va_arg(ap, unsigned int);
            print_uint(v, 2, width, pad, false);
            break;
        }
        case 'p': {
            uintptr_t v = (uintptr_t)__builtin_va_arg(ap, void *);
            print_str("0x");
            print_uint(v, 16, 16, '0', false);
            break;
        }
        case 'c':
            serial_write_char((char)__builtin_va_arg(ap, int));
            break;
        case 's':
            print_str(__builtin_va_arg(ap, const char *));
            break;
        case '%':
            serial_write_char('%');
            break;
        default:
            serial_write_char('%');
            serial_write_char(*fmt);
            break;
        }
    }
}

void printk(const char *fmt, ...) {
    __builtin_va_list ap;
    __builtin_va_start(ap, fmt);
    vprintk(fmt, ap);
    __builtin_va_end(ap);
}

/* ─── Kernel panic ───────────────────────────────────────────────────────── */

void kernel_panic(const char *msg) {
    /* Disable interrupts immediately */
    __asm__ volatile("cli");

    printk("\n\n");
    printk("╔══════════════════════════════════════════════════════╗\n");
    printk("║             *** HEROOS KERNEL PANIC ***              ║\n");
    printk("╠══════════════════════════════════════════════════════╣\n");
    printk("║ %s\n", msg);
    printk("╚══════════════════════════════════════════════════════╝\n");
    printk("System halted. Please reset your machine.\n");

    /* Halt forever */
    for (;;) __asm__ volatile("hlt");
}
