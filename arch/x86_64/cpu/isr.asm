; ISR and IRQ stubs for x86_64
;
; Each ISR stub:
;   1. Pushes a dummy error code (for exceptions that don't push one)
;   2. Pushes the interrupt number
;   3. Jumps to common_stub which saves all GPRs and calls isr_common_handler(frame)
;
; The C handler receives a pointer to interrupt_frame_t.

BITS 64
section .text

extern isr_common_handler
extern irq_common_handler

; ─── Common ISR stub ──────────────────────────────────────────────────────────
isr_common_stub:
    ; Save all general-purpose registers (order matches interrupt_frame_t)
    push    rax
    push    rbx
    push    rcx
    push    rdx
    push    rsi
    push    rdi
    push    rbp
    push    r8
    push    r9
    push    r10
    push    r11
    push    r12
    push    r13
    push    r14
    push    r15

    ; Pass pointer to interrupt frame as first argument
    mov     rdi, rsp
    call    isr_common_handler

    ; Restore registers
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     r11
    pop     r10
    pop     r9
    pop     r8
    pop     rbp
    pop     rdi
    pop     rsi
    pop     rdx
    pop     rcx
    pop     rbx
    pop     rax

    ; Remove int_no and err_code from stack
    add     rsp, 16
    iretq

; ─── Common IRQ stub ──────────────────────────────────────────────────────────
irq_common_stub:
    push    rax
    push    rbx
    push    rcx
    push    rdx
    push    rsi
    push    rdi
    push    rbp
    push    r8
    push    r9
    push    r10
    push    r11
    push    r12
    push    r13
    push    r14
    push    r15

    mov     rdi, rsp
    call    irq_common_handler

    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     r11
    pop     r10
    pop     r9
    pop     r8
    pop     rbp
    pop     rdi
    pop     rsi
    pop     rdx
    pop     rcx
    pop     rbx
    pop     rax

    add     rsp, 16
    iretq

; ─── ISR macro helpers ────────────────────────────────────────────────────────
; ISR without error code (CPU doesn't push one)
%macro ISR_NOERR 1
global isr%1
isr%1:
    push    qword 0      ; dummy error code
    push    qword %1     ; interrupt number
    jmp     isr_common_stub
%endmacro

; ISR with error code (CPU pushes one automatically)
%macro ISR_ERR 1
global isr%1
isr%1:
    push    qword %1     ; interrupt number (error code already on stack)
    jmp     isr_common_stub
%endmacro

; IRQ stub
%macro IRQ_STUB 2
global irq%1
irq%1:
    push    qword 0      ; dummy error code
    push    qword %2     ; mapped interrupt vector
    jmp     irq_common_stub
%endmacro

; ─── CPU Exceptions (vectors 0–20) ────────────────────────────────────────────
ISR_NOERR  0    ; Divide by zero
ISR_NOERR  1    ; Debug
ISR_NOERR  2    ; Non-maskable interrupt
ISR_NOERR  3    ; Breakpoint
ISR_NOERR  4    ; Overflow
ISR_NOERR  5    ; Bound range exceeded
ISR_NOERR  6    ; Invalid opcode
ISR_NOERR  7    ; Device not available
ISR_ERR    8    ; Double fault (error code = 0)
ISR_NOERR  9    ; Coprocessor segment overrun (reserved)
ISR_ERR    10   ; Invalid TSS
ISR_ERR    11   ; Segment not present
ISR_ERR    12   ; Stack-segment fault
ISR_ERR    13   ; General protection fault
ISR_ERR    14   ; Page fault
ISR_NOERR  15   ; Reserved
ISR_NOERR  16   ; x87 FPU exception
ISR_ERR    17   ; Alignment check
ISR_NOERR  18   ; Machine check
ISR_NOERR  19   ; SIMD floating-point exception
ISR_NOERR  20   ; Virtualization exception

; Syscall via int 0x80
ISR_NOERR  128

; ─── Hardware IRQs (remapped to vectors 0x20–0x2F) ────────────────────────────
IRQ_STUB   0,  0x20    ; PIT Timer
IRQ_STUB   1,  0x21    ; Keyboard
IRQ_STUB   2,  0x22    ; Cascade (used internally)
IRQ_STUB   3,  0x23    ; COM2
IRQ_STUB   4,  0x24    ; COM1
IRQ_STUB   5,  0x25    ; LPT2
IRQ_STUB   6,  0x26    ; Floppy disk
IRQ_STUB   7,  0x27    ; LPT1 / spurious
IRQ_STUB   8,  0x28    ; CMOS RTC
IRQ_STUB   9,  0x29    ; Free / ACPI
IRQ_STUB   10, 0x2A    ; Free
IRQ_STUB   11, 0x2B    ; Free
IRQ_STUB   12, 0x2C    ; PS/2 Mouse
IRQ_STUB   13, 0x2D    ; FPU
IRQ_STUB   14, 0x2E    ; Primary ATA disk
IRQ_STUB   15, 0x2F    ; Secondary ATA disk
