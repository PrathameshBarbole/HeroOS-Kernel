; HeroOS x86_64 Boot Entry
; Multiboot2-compliant bootloader header + long-mode setup
;
; Boot sequence:
;   1. GRUB loads this in 32-bit protected mode (multiboot2)
;   2. We verify CPUID / long-mode support
;   3. Set up initial page tables (identity-map first 2 GiB + higher-half kernel)
;   4. Enable PAE, long mode (EFER.LME), and paging (CR0.PG)
;   5. Long-jump into 64-bit code segment
;   6. Set up 64-bit stack and call kmain()

BITS 32

; ─── Multiboot2 Header ────────────────────────────────────────────────────────
section .multiboot2
ALIGN 8
mb2_header_start:
    dd  0xE85250D6              ; Multiboot2 magic
    dd  0                       ; Architecture: i386 (32-bit protected mode)
    dd  mb2_header_end - mb2_header_start
    dd  -(0xE85250D6 + 0 + (mb2_header_end - mb2_header_start)) ; checksum

    ; Framebuffer request tag
    ALIGN 8
    dw  5                       ; Tag type: framebuffer
    dw  1                       ; Optional flag
    dd  20                      ; Size
    dd  0                       ; Width  (0 = don't care)
    dd  0                       ; Height (0 = don't care)
    dd  32                      ; Depth  (32 bpp)

    ; End tag
    ALIGN 8
    dw  0                       ; Tag type: end
    dw  0
    dd  8
mb2_header_end:

; ─── 32-bit startup code ──────────────────────────────────────────────────────
section .boot32
global _start
_start:
    ; Disable interrupts and set up a temporary stack
    cli
    mov     esp, stack32_top

    ; Save multiboot2 info pointer (ebx) for later passing to kmain
    mov     [mb2_info_ptr], ebx

    ; Check that we were loaded by a Multiboot2-compatible bootloader
    cmp     eax, 0x36D76289
    jne     .no_multiboot

    ; ── Check CPUID support ────────────────────────────────────────────────
    ; Try to flip bit 21 of EFLAGS (ID bit)
    pushfd
    pop     eax
    mov     ecx, eax
    xor     eax, (1 << 21)
    push    eax
    popfd
    pushfd
    pop     eax
    push    ecx
    popfd
    xor     eax, ecx
    jz      .no_cpuid

    ; ── Check long mode support ────────────────────────────────────────────
    mov     eax, 0x80000000
    cpuid
    cmp     eax, 0x80000001
    jb      .no_long_mode

    mov     eax, 0x80000001
    cpuid
    test    edx, (1 << 29)      ; LM bit
    jz      .no_long_mode

    ; ── Set up page tables ─────────────────────────────────────────────────
    ; PML4  @ pml4_table
    ; PDPT  @ pdpt_table
    ; PD    @ pd_table (2 MiB pages — simpler, no PT needed)
    ;
    ; We map:
    ;   Virtual 0x0000000000000000 → physical 0x00000000 (identity, first 2 GiB)
    ;   Virtual 0xFFFFFFFF80000000 → physical 0x00000000 (higher-half kernel)

    ; Clear page tables
    mov     edi, pml4_table
    mov     ecx, (3 * 512)      ; PML4 + PDPT + PD = 3 tables × 512 entries
    xor     eax, eax
    rep stosd

    ; PML4[0] → PDPT (identity mapping)
    mov     eax, pdpt_low
    or      eax, 0x03           ; Present + Writable
    mov     [pml4_table], eax

    ; PML4[511] → PDPT (higher-half)
    mov     eax, pdpt_high
    or      eax, 0x03
    mov     [pml4_table + 511*8], eax

    ; PDPT_low[0] → PD
    mov     eax, pd_table
    or      eax, 0x03
    mov     [pdpt_low], eax

    ; PDPT_high[510] → PD  (maps VA 0xFFFFFFFF80000000)
    mov     eax, pd_table
    or      eax, 0x03
    mov     [pdpt_high + 510*8], eax

    ; PD: map first 2 GiB as 2 MiB huge pages
    mov     edi, pd_table
    mov     eax, 0x83           ; Present + Writable + Huge
    mov     ecx, 1024           ; 1024 × 2 MiB = 2 GiB
.fill_pd:
    mov     [edi], eax
    add     eax, 0x200000       ; Next 2 MiB
    add     edi, 8
    loop    .fill_pd

    ; ── Enable PAE ────────────────────────────────────────────────────────
    mov     eax, cr4
    or      eax, (1 << 5)       ; CR4.PAE
    mov     cr4, eax

    ; ── Load PML4 into CR3 ────────────────────────────────────────────────
    mov     eax, pml4_table
    mov     cr3, eax

    ; ── Enable Long Mode in EFER ──────────────────────────────────────────
    mov     ecx, 0xC0000080     ; EFER MSR
    rdmsr
    or      eax, (1 << 8)       ; EFER.LME
    or      eax, (1 << 11)      ; EFER.NXE (No-Execute Enable)
    wrmsr

    ; ── Enable Paging (enter compatibility mode → long mode on far jump) ──
    mov     eax, cr0
    or      eax, (1 << 31) | (1 << 0)   ; CR0.PG + CR0.PE
    mov     cr0, eax

    ; ── Load 64-bit GDT and far jump to 64-bit code ───────────────────────
    lgdt    [gdt64_ptr]
    jmp     0x08:.long_mode_entry

.no_multiboot:
    mov     dword [0xB8000], 0x4F4E4F4D   ; "MN" in red on screen
    jmp     $

.no_cpuid:
    mov     dword [0xB8000], 0x4F43      ; 'C' in red
    jmp     $

.no_long_mode:
    mov     dword [0xB8000], 0x4F4C      ; 'L' in red
    jmp     $

; ─── 64-bit long mode entry ───────────────────────────────────────────────────
BITS 64
section .text
extern kmain
global _start64

_start64:
.long_mode_entry:
    ; Reload segment registers with 64-bit data selector
    mov     ax, 0x10
    mov     ds, ax
    mov     es, ax
    mov     fs, ax
    mov     gs, ax
    mov     ss, ax

    ; Switch to 64-bit kernel stack (higher-half virtual address)
    mov     rsp, kernel_stack_top

    ; Clear the direction flag
    cld

    ; Load multiboot2 info pointer into rdi (first argument to kmain)
    mov     rdi, qword [mb2_info_ptr]

    ; Call the C kernel entry point
    call    kmain

    ; If kmain ever returns, halt forever
.halt:
    cli
    hlt
    jmp     .halt

; ─── 64-bit GDT (minimal: null, code64, data64) ───────────────────────────────
section .rodata
ALIGN 8
gdt64:
    dq  0x0000000000000000           ; 0x00: null descriptor
    dq  0x00AF9A000000FFFF           ; 0x08: 64-bit kernel code (L=1, P=1, DPL=0)
    dq  0x00AF92000000FFFF           ; 0x10: 64-bit kernel data (P=1, DPL=0, S=1, RW=1)
gdt64_end:

gdt64_ptr:
    dw  gdt64_end - gdt64 - 1
    dq  gdt64

; ─── Early page tables (BSS — zero-initialised by bootloader) ─────────────────
section .bss
ALIGN 0x1000

global pml4_table
global pdpt_low
global pdpt_high
global pd_table

pml4_table: resb 0x1000
pdpt_low:   resb 0x1000
pdpt_high:  resb 0x1000
pd_table:   resb 0x2000    ; 2 pages for 1024 entries

; 32-bit temporary stack
stack32: resb 4096
stack32_top:

; 64-bit kernel stack (16 KiB)
ALIGN 16
kernel_stack: resb 16384
kernel_stack_top:

; Multiboot2 info pointer storage
mb2_info_ptr: resq 1
