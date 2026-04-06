; Context switch for x86_64
;
; void context_switch(cpu_context_t *old_ctx, cpu_context_t *new_ctx)
;   rdi = old_ctx  (save current registers here)
;   rsi = new_ctx  (restore registers from here)
;
; The layout of cpu_context_t (from process.h):
;   r15(0) r14(8) r13(16) r12(24) r11(32) r10(40) r9(48) r8(56)
;   rbp(64) rdi(72) rsi(80) rdx(88) rcx(96) rbx(104) rax(112)
;   rip(120) rflags(128) rsp(136) cs(144) ss(152)

BITS 64
section .text

global context_switch
context_switch:
    ; ── Save current context into old_ctx ────────────────────────────────
    mov     [rdi + 0],   r15
    mov     [rdi + 8],   r14
    mov     [rdi + 16],  r13
    mov     [rdi + 24],  r12
    mov     [rdi + 32],  r11
    mov     [rdi + 40],  r10
    mov     [rdi + 48],  r9
    mov     [rdi + 56],  r8
    mov     [rdi + 64],  rbp
    mov     [rdi + 72],  rdi    ; Note: rdi already holds old_ctx address
    mov     [rdi + 80],  rsi    ; Note: rsi already holds new_ctx address
    mov     [rdi + 88],  rdx
    mov     [rdi + 96],  rcx
    mov     [rdi + 104], rbx
    mov     [rdi + 112], rax

    ; Save instruction pointer (use return address as RIP)
    mov     rax, [rsp]
    mov     [rdi + 120], rax    ; rip

    ; Save RFLAGS
    pushfq
    pop     rax
    mov     [rdi + 128], rax    ; rflags

    ; Save RSP (pointing to the return address slot)
    lea     rax, [rsp + 8]      ; rsp after return
    mov     [rdi + 136], rax

    ; ── Restore new context from new_ctx ─────────────────────────────────
    mov     r15, [rsi + 0]
    mov     r14, [rsi + 8]
    mov     r13, [rsi + 16]
    mov     r12, [rsi + 24]
    mov     r11, [rsi + 32]
    mov     r10, [rsi + 40]
    mov     r9,  [rsi + 48]
    mov     r8,  [rsi + 56]
    mov     rbp, [rsi + 64]
    mov     rdx, [rsi + 88]
    mov     rcx, [rsi + 96]
    mov     rbx, [rsi + 104]
    mov     rax, [rsi + 112]

    ; Restore stack pointer
    mov     rsp, [rsi + 136]

    ; Restore RFLAGS
    push    qword [rsi + 128]
    popfq

    ; Push new RIP and return to it
    push    qword [rsi + 120]

    ; Restore rdi and rsi last (we were using them as pointers)
    mov     rdi, [rsi + 72]
    mov     rsi, [rsi + 80]

    ret
