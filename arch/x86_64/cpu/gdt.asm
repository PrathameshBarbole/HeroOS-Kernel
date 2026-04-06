; GDT flush and TSS load stubs for x86_64

BITS 64
section .text

; void gdt_flush(uint64_t gdt_ptr)
; Loads the GDT from the address passed in rdi, then reloads all
; segment registers.  CS is reloaded via a far return.
global gdt_flush
gdt_flush:
    lgdt    [rdi]               ; Load GDTR

    ; Reload data segments
    mov     ax, 0x10            ; Kernel data selector
    mov     ds, ax
    mov     es, ax
    mov     fs, ax
    mov     gs, ax
    mov     ss, ax

    ; Reload CS via far return trick
    pop     rax                 ; Return address
    push    qword 0x08         ; Kernel code selector
    push    rax
    retfq                       ; Far return → jumps with new CS

; void tss_flush(void)
; Loads the TSS selector into the task register.
global tss_flush
tss_flush:
    mov     ax, 0x28            ; TSS selector (GDT offset 0x28)
    ltr     ax
    ret
