; IDT flush stub for x86_64

BITS 64
section .text

; void idt_flush(uint64_t idt_ptr)
global idt_flush
idt_flush:
    lidt    [rdi]
    ret
