#include "idt.h"
#include <lib/string.h>
#include <kernel/printk.h>

/* ─── PIC (8259A) remapping ──────────────────────────────────────────────── */

#define PIC1_COMMAND  0x20
#define PIC1_DATA     0x21
#define PIC2_COMMAND  0xA0
#define PIC2_DATA     0xA1
#define PIC_EOI       0x20

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}
static inline uint8_t inb(uint16_t port) {
    uint8_t v; __asm__ volatile("inb %1, %0" : "=a"(v) : "Nd"(port)); return v;
}
static inline void io_wait(void) { outb(0x80, 0); }

static void pic_remap(uint8_t offset1, uint8_t offset2) {
    uint8_t mask1 = inb(PIC1_DATA);
    uint8_t mask2 = inb(PIC2_DATA);

    outb(PIC1_COMMAND, 0x11); io_wait(); /* ICW1: init + ICW4 needed */
    outb(PIC2_COMMAND, 0x11); io_wait();
    outb(PIC1_DATA, offset1); io_wait(); /* ICW2: vector offset */
    outb(PIC2_DATA, offset2); io_wait();
    outb(PIC1_DATA, 0x04);    io_wait(); /* ICW3: slave on IRQ2 */
    outb(PIC2_DATA, 0x02);    io_wait(); /* ICW3: slave cascade identity */
    outb(PIC1_DATA, 0x01);    io_wait(); /* ICW4: 8086 mode */
    outb(PIC2_DATA, 0x01);    io_wait();

    outb(PIC1_DATA, mask1);
    outb(PIC2_DATA, mask2);
}

static void pic_send_eoi(uint8_t irq) {
    if (irq >= 8) outb(PIC2_COMMAND, PIC_EOI);
    outb(PIC1_COMMAND, PIC_EOI);
}

/* ─── IDT storage ────────────────────────────────────────────────────────── */

static idt_entry_t   idt[IDT_ENTRY_COUNT];
static idt_ptr_t     idt_ptr_val;
static irq_handler_t irq_handlers[IRQ_COUNT];

/* ─── Install a gate ─────────────────────────────────────────────────────── */

void idt_set_handler(uint8_t vector, uintptr_t handler, uint8_t type_attr, uint8_t ist) {
    idt[vector].offset_low  = (uint16_t)(handler & 0xFFFF);
    idt[vector].offset_mid  = (uint16_t)((handler >> 16) & 0xFFFF);
    idt[vector].offset_high = (uint32_t)((handler >> 32) & 0xFFFFFFFF);
    idt[vector].selector    = GDT_KERNEL_CODE;
    idt[vector].ist         = ist & 0x07;
    idt[vector].type_attr   = type_attr;
    idt[vector].reserved    = 0;
}

/* ─── Exception names ────────────────────────────────────────────────────── */

static const char *exception_names[] = {
    "Divide by Zero",          "Debug",                "Non-Maskable Interrupt",
    "Breakpoint",              "Overflow",             "Bound Range Exceeded",
    "Invalid Opcode",          "Device Not Available", "Double Fault",
    "Coprocessor Seg Overrun", "Invalid TSS",          "Segment Not Present",
    "Stack-Segment Fault",     "General Protection",   "Page Fault",
    "Reserved",                "x87 FPU Exception",    "Alignment Check",
    "Machine Check",           "SIMD Exception",       "Virtualization Exception"
};

/* ─── Common exception/ISR handler (called from isr.asm) ─────────────────── */

void isr_common_handler(interrupt_frame_t *frame) {
    if (frame->int_no == 128) {
        /* Syscall via int 0x80 — handled separately */
        extern void syscall_handler(void *frame);
        syscall_handler(frame);
        return;
    }

    const char *name = (frame->int_no < 21)
                       ? exception_names[frame->int_no]
                       : "Unknown Exception";

    pr_err("=== CPU EXCEPTION #%llu: %s ===\n", frame->int_no, name);
    pr_err("  Error code : 0x%016llx\n", frame->err_code);
    pr_err("  RIP        : 0x%016llx\n", frame->rip);
    pr_err("  CS         : 0x%04llx\n",  frame->cs);
    pr_err("  RFLAGS     : 0x%016llx\n", frame->rflags);
    pr_err("  RSP        : 0x%016llx\n", frame->rsp);
    pr_err("  SS         : 0x%04llx\n",  frame->ss);
    pr_err("  RAX=0x%llx RBX=0x%llx RCX=0x%llx RDX=0x%llx\n",
           frame->rax, frame->rbx, frame->rcx, frame->rdx);

    if (frame->int_no == EXC_PAGE_FAULT) {
        uintptr_t cr2;
        __asm__ volatile("mov %%cr2, %0" : "=r"(cr2));
        pr_err("  CR2 (fault address): 0x%016llx\n", (uint64_t)cr2);
        pr_err("  Fault flags: %s %s %s %s\n",
               (frame->err_code & 1) ? "PRESENT"    : "NOT_PRESENT",
               (frame->err_code & 2) ? "WRITE"      : "READ",
               (frame->err_code & 4) ? "USER"       : "KERNEL",
               (frame->err_code & 8) ? "RSVD_WRITE" : "");
    }

    kernel_panic(name);
}

/* ─── Common IRQ handler (called from isr.asm) ───────────────────────────── */

void irq_common_handler(interrupt_frame_t *frame) {
    uint8_t irq = (uint8_t)(frame->int_no - IRQ_BASE);
    if (irq < IRQ_COUNT && irq_handlers[irq]) {
        irq_handlers[irq](frame);
    }
    pic_send_eoi(irq);
}

/* ─── IRQ handler registration ───────────────────────────────────────────── */

void irq_register_handler(uint8_t irq, irq_handler_t handler) {
    if (irq < IRQ_COUNT) irq_handlers[irq] = handler;
}

void irq_unregister_handler(uint8_t irq) {
    if (irq < IRQ_COUNT) irq_handlers[irq] = NULL;
}

void irq_enable(void)  { __asm__ volatile("sti"); }
void irq_disable(void) { __asm__ volatile("cli"); }

/* ─── IDT initialisation ─────────────────────────────────────────────────── */

void idt_init(void) {
    memset(idt, 0, sizeof(idt));
    memset(irq_handlers, 0, sizeof(irq_handlers));

    /* Remap PIC to vectors 0x20–0x2F */
    pic_remap(IRQ_BASE, IRQ_BASE + 8);

    /* Install exception ISRs */
    idt_set_handler(0,  (uintptr_t)isr0,  IDT_GATE_TRAP,      0);
    idt_set_handler(1,  (uintptr_t)isr1,  IDT_GATE_TRAP,      0);
    idt_set_handler(2,  (uintptr_t)isr2,  IDT_GATE_INTERRUPT, 0);
    idt_set_handler(3,  (uintptr_t)isr3,  IDT_GATE_TRAP,      0);
    idt_set_handler(4,  (uintptr_t)isr4,  IDT_GATE_TRAP,      0);
    idt_set_handler(5,  (uintptr_t)isr5,  IDT_GATE_TRAP,      0);
    idt_set_handler(6,  (uintptr_t)isr6,  IDT_GATE_TRAP,      0);
    idt_set_handler(7,  (uintptr_t)isr7,  IDT_GATE_TRAP,      0);
    idt_set_handler(8,  (uintptr_t)isr8,  IDT_GATE_INTERRUPT, 1); /* Double fault — IST 1 */
    idt_set_handler(10, (uintptr_t)isr10, IDT_GATE_TRAP,      0);
    idt_set_handler(11, (uintptr_t)isr11, IDT_GATE_TRAP,      0);
    idt_set_handler(12, (uintptr_t)isr12, IDT_GATE_TRAP,      0);
    idt_set_handler(13, (uintptr_t)isr13, IDT_GATE_TRAP,      0);
    idt_set_handler(14, (uintptr_t)isr14, IDT_GATE_TRAP,      0);
    idt_set_handler(15, (uintptr_t)isr15, IDT_GATE_TRAP,      0);
    idt_set_handler(16, (uintptr_t)isr16, IDT_GATE_TRAP,      0);
    idt_set_handler(17, (uintptr_t)isr17, IDT_GATE_TRAP,      0);
    idt_set_handler(18, (uintptr_t)isr18, IDT_GATE_INTERRUPT, 0);
    idt_set_handler(19, (uintptr_t)isr19, IDT_GATE_TRAP,      0);
    idt_set_handler(20, (uintptr_t)isr20, IDT_GATE_TRAP,      0);

    /* Syscall gate (int 0x80) — accessible from ring 3 */
    idt_set_handler(128, (uintptr_t)isr128, IDT_GATE_USER, 0);

    /* Install hardware IRQ handlers */
    idt_set_handler(IRQ(0),  (uintptr_t)irq0,  IDT_GATE_INTERRUPT, 0);
    idt_set_handler(IRQ(1),  (uintptr_t)irq1,  IDT_GATE_INTERRUPT, 0);
    idt_set_handler(IRQ(2),  (uintptr_t)irq2,  IDT_GATE_INTERRUPT, 0);
    idt_set_handler(IRQ(3),  (uintptr_t)irq3,  IDT_GATE_INTERRUPT, 0);
    idt_set_handler(IRQ(4),  (uintptr_t)irq4,  IDT_GATE_INTERRUPT, 0);
    idt_set_handler(IRQ(5),  (uintptr_t)irq5,  IDT_GATE_INTERRUPT, 0);
    idt_set_handler(IRQ(6),  (uintptr_t)irq6,  IDT_GATE_INTERRUPT, 0);
    idt_set_handler(IRQ(7),  (uintptr_t)irq7,  IDT_GATE_INTERRUPT, 0);
    idt_set_handler(IRQ(8),  (uintptr_t)irq8,  IDT_GATE_INTERRUPT, 0);
    idt_set_handler(IRQ(9),  (uintptr_t)irq9,  IDT_GATE_INTERRUPT, 0);
    idt_set_handler(IRQ(10), (uintptr_t)irq10, IDT_GATE_INTERRUPT, 0);
    idt_set_handler(IRQ(11), (uintptr_t)irq11, IDT_GATE_INTERRUPT, 0);
    idt_set_handler(IRQ(12), (uintptr_t)irq12, IDT_GATE_INTERRUPT, 0);
    idt_set_handler(IRQ(13), (uintptr_t)irq13, IDT_GATE_INTERRUPT, 0);
    idt_set_handler(IRQ(14), (uintptr_t)irq14, IDT_GATE_INTERRUPT, 0);
    idt_set_handler(IRQ(15), (uintptr_t)irq15, IDT_GATE_INTERRUPT, 0);

    /* Load IDTR */
    idt_ptr_val.limit = (uint16_t)(sizeof(idt) - 1);
    idt_ptr_val.base  = (uint64_t)(uintptr_t)idt;
    idt_flush((uint64_t)(uintptr_t)&idt_ptr_val);

    pr_info("IDT initialised (256 entries, PIC remapped to 0x%02x-0x%02x)\n",
            IRQ_BASE, IRQ_BASE + 15);
}
