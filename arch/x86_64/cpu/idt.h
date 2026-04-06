#ifndef ARCH_IDT_H
#define ARCH_IDT_H

#include <kernel/types.h>

#define IDT_ENTRY_COUNT   256

/* Gate types */
#define IDT_GATE_INTERRUPT  0x8E  /* Present, Ring 0, 64-bit interrupt gate */
#define IDT_GATE_TRAP       0x8F  /* Present, Ring 0, 64-bit trap gate */
#define IDT_GATE_USER       0xEE  /* Present, Ring 3, 64-bit interrupt gate */

/* Standard x86 exception vectors */
#define EXC_DIVIDE_BY_ZERO    0
#define EXC_DEBUG             1
#define EXC_NMI               2
#define EXC_BREAKPOINT        3
#define EXC_OVERFLOW          4
#define EXC_BOUND_RANGE       5
#define EXC_INVALID_OPCODE    6
#define EXC_DEVICE_NOT_AVAIL  7
#define EXC_DOUBLE_FAULT      8
#define EXC_INVALID_TSS       10
#define EXC_SEG_NOT_PRESENT   11
#define EXC_STACK_SEG_FAULT   12
#define EXC_GENERAL_PROTECT   13
#define EXC_PAGE_FAULT        14
#define EXC_FPU_EXCEPTION     16
#define EXC_ALIGNMENT_CHECK   17
#define EXC_MACHINE_CHECK     18
#define EXC_SIMD_EXCEPTION    19
#define EXC_VIRT_EXCEPTION    20
#define EXC_SECURITY          30

/* IRQ base vector (remapped PIC) */
#define IRQ_BASE    0x20
#define IRQ_COUNT   16
#define IRQ(n)      (IRQ_BASE + (n))

/* IDT entry (16 bytes) */
typedef struct PACKED {
    uint16_t  offset_low;
    uint16_t  selector;
    uint8_t   ist;         /* Interrupt Stack Table index (0 = none) */
    uint8_t   type_attr;
    uint16_t  offset_mid;
    uint32_t  offset_high;
    uint32_t  reserved;
} idt_entry_t;

/* IDT pointer (IDTR register value) */
typedef struct PACKED {
    uint16_t  limit;
    uint64_t  base;
} idt_ptr_t;

/* Interrupt frame pushed by CPU + ISR stubs */
typedef struct {
    /* General purpose registers (pushed by isr.asm) */
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    /* Interrupt info (pushed by ISR stub) */
    uint64_t int_no;
    uint64_t err_code;
    /* CPU-pushed fields */
    uint64_t rip, cs, rflags, rsp, ss;
} interrupt_frame_t;

/* IRQ handler function type */
typedef void (*irq_handler_t)(interrupt_frame_t *frame);

void idt_init(void);
void idt_set_handler(uint8_t vector, uintptr_t handler, uint8_t type_attr, uint8_t ist);
void irq_register_handler(uint8_t irq, irq_handler_t handler);
void irq_unregister_handler(uint8_t irq);
void irq_enable(void);
void irq_disable(void);

/* Defined in idt.asm */
extern void idt_flush(uint64_t idt_ptr);

/* ISR stubs defined in isr.asm */
extern void isr0(void);  extern void isr1(void);  extern void isr2(void);
extern void isr3(void);  extern void isr4(void);  extern void isr5(void);
extern void isr6(void);  extern void isr7(void);  extern void isr8(void);
extern void isr9(void);  extern void isr10(void); extern void isr11(void);
extern void isr12(void); extern void isr13(void); extern void isr14(void);
extern void isr15(void); extern void isr16(void); extern void isr17(void);
extern void isr18(void); extern void isr19(void); extern void isr20(void);
extern void isr128(void); /* syscall */

/* IRQ stubs */
extern void irq0(void);  extern void irq1(void);  extern void irq2(void);
extern void irq3(void);  extern void irq4(void);  extern void irq5(void);
extern void irq6(void);  extern void irq7(void);  extern void irq8(void);
extern void irq9(void);  extern void irq10(void); extern void irq11(void);
extern void irq12(void); extern void irq13(void); extern void irq14(void);
extern void irq15(void);

#endif /* ARCH_IDT_H */
