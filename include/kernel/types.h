#ifndef KERNEL_TYPES_H
#define KERNEL_TYPES_H

/* Fundamental integer types */
typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;

typedef signed char        int8_t;
typedef signed short       int16_t;
typedef signed int         int32_t;
typedef signed long long   int64_t;

typedef uint64_t           uintptr_t;
typedef int64_t            intptr_t;
typedef uint64_t           size_t;
typedef int64_t            ssize_t;
typedef int64_t            off_t;
typedef uint32_t           pid_t;
typedef uint32_t           uid_t;
typedef uint32_t           gid_t;
typedef int32_t            status_t;

/* Boolean */
typedef uint8_t            bool;
#define true  1
#define false 0

/* NULL */
#ifndef NULL
#define NULL ((void *)0)
#endif

/* Limits */
#define UINT8_MAX   0xFFU
#define UINT16_MAX  0xFFFFU
#define UINT32_MAX  0xFFFFFFFFU
#define UINT64_MAX  0xFFFFFFFFFFFFFFFFULL

/* Compiler helpers */
#define PACKED      __attribute__((packed))
#define ALIGNED(n)  __attribute__((aligned(n)))
#define NORETURN    __attribute__((noreturn))
#define UNUSED      __attribute__((unused))
#define WEAK        __attribute__((weak))
#define ALWAYS_INLINE __attribute__((always_inline)) inline
#define SECTION(s)  __attribute__((section(s)))

/* Array size */
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

/* Min/Max */
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

/* Alignment helpers */
#define ALIGN_UP(x, a)   (((x) + (a) - 1) & ~((a) - 1))
#define ALIGN_DOWN(x, a) ((x) & ~((a) - 1))

/* Bit manipulation */
#define BIT(n)          (1ULL << (n))
#define BIT_SET(x, n)   ((x) |=  BIT(n))
#define BIT_CLR(x, n)   ((x) &= ~BIT(n))
#define BIT_TEST(x, n)  ((x) &   BIT(n))

/* Page size constants */
#define PAGE_SIZE        0x1000UL          /* 4 KiB */
#define PAGE_SHIFT       12
#define LARGE_PAGE_SIZE  0x200000UL        /* 2 MiB */
#define HUGE_PAGE_SIZE   0x40000000UL      /* 1 GiB */

/* Kernel virtual base address (higher-half kernel) */
#define KERNEL_VIRT_BASE 0xFFFFFFFF80000000ULL
#define KERNEL_PHYS_BASE 0x0000000001000000ULL  /* 1 MiB physical load address */

/* Convert between physical and virtual (kernel space) */
#define PHYS_TO_VIRT(addr) ((void *)((uintptr_t)(addr) + KERNEL_VIRT_BASE))
#define VIRT_TO_PHYS(addr) ((uintptr_t)(addr) - KERNEL_VIRT_BASE)

#endif /* KERNEL_TYPES_H */
