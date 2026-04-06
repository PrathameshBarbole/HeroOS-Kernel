#ifndef KERNEL_PRINTK_H
#define KERNEL_PRINTK_H

#include <kernel/types.h>

/* Log levels */
#define LOG_EMERG   0   /* System is unusable */
#define LOG_ALERT   1   /* Action must be taken immediately */
#define LOG_CRIT    2   /* Critical conditions */
#define LOG_ERR     3   /* Error conditions */
#define LOG_WARN    4   /* Warning conditions */
#define LOG_NOTICE  5   /* Normal but significant condition */
#define LOG_INFO    6   /* Informational */
#define LOG_DEBUG   7   /* Debug-level messages */

/* Default log level threshold */
#ifndef KERNEL_LOG_LEVEL
#define KERNEL_LOG_LEVEL LOG_DEBUG
#endif

/* Core print functions */
void printk(const char *fmt, ...);
void vprintk(const char *fmt, __builtin_va_list ap);

/* Level-prefixed helpers */
#define pr_emerg(fmt, ...)  printk("[EMERG]  " fmt, ##__VA_ARGS__)
#define pr_alert(fmt, ...)  printk("[ALERT]  " fmt, ##__VA_ARGS__)
#define pr_crit(fmt, ...)   printk("[CRIT]   " fmt, ##__VA_ARGS__)
#define pr_err(fmt, ...)    printk("[ERROR]  " fmt, ##__VA_ARGS__)
#define pr_warn(fmt, ...)   printk("[WARN]   " fmt, ##__VA_ARGS__)
#define pr_notice(fmt, ...) printk("[NOTICE] " fmt, ##__VA_ARGS__)
#define pr_info(fmt, ...)   printk("[INFO]   " fmt, ##__VA_ARGS__)
#define pr_debug(fmt, ...)  printk("[DEBUG]  " fmt, ##__VA_ARGS__)

/* Assertion */
#define KASSERT(cond, msg)                                    \
    do {                                                      \
        if (!(cond)) {                                        \
            printk("[ASSERT FAILED] %s:%d: %s\n",            \
                   __FILE__, __LINE__, msg);                  \
            kernel_panic("Assertion failed: " msg);           \
        }                                                     \
    } while (0)

void kernel_panic(const char *msg) NORETURN;

#endif /* KERNEL_PRINTK_H */
