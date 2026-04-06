/*
 * HeroOS x86_64 Linker Script
 *
 * Physical load address:  0x0000000001000000  (1 MiB)
 * Virtual (higher-half):  0xFFFFFFFF80000000
 *
 * The kernel is loaded physically at 1 MiB by GRUB but executes from the
 * higher-half virtual address.  boot.asm sets up the initial page tables that
 * map both the identity (0→0) and higher-half regions before jumping to C.
 */

ENTRY(_start)
OUTPUT_FORMAT("elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)

KERNEL_VIRT_BASE = 0xFFFFFFFF80000000;
KERNEL_PHYS_BASE = 0x0000000001000000;

SECTIONS {
    . = KERNEL_PHYS_BASE;

    /* ── Multiboot2 header (must be within first 32 KiB of ELF) ── */
    .multiboot2 ALIGN(8) : {
        *(.multiboot2)
    }

    /* ── 32-bit boot code (must stay below 4 GiB) ── */
    .boot32 ALIGN(4096) : {
        *(.boot32)
    }

    /* Adjust to higher-half virtual base for the rest */
    . += KERNEL_VIRT_BASE;

    /* ── Kernel text ── */
    .text ALIGN(4096) : AT(ADDR(.text) - KERNEL_VIRT_BASE) {
        *(.text)
        *(.text.*)
    }

    /* ── Read-only data ── */
    .rodata ALIGN(4096) : AT(ADDR(.rodata) - KERNEL_VIRT_BASE) {
        *(.rodata)
        *(.rodata.*)
    }

    /* ── Initialised data ── */
    .data ALIGN(4096) : AT(ADDR(.data) - KERNEL_VIRT_BASE) {
        *(.data)
        *(.data.*)
    }

    /* ── Block Starting Symbol (zero-init) ── */
    .bss ALIGN(4096) : AT(ADDR(.bss) - KERNEL_VIRT_BASE) {
        *(COMMON)
        *(.bss)
        *(.bss.*)
    }

    /* Discard debug / comment sections to keep image small */
    /DISCARD/ : {
        *(.comment)
        *(.eh_frame)
        *(.note*)
    }

    /* Symbols exported for kernel use */
    _kernel_start  = KERNEL_PHYS_BASE;
    _kernel_end    = . - KERNEL_VIRT_BASE;
    _kernel_size   = _kernel_end - _kernel_start;
}
