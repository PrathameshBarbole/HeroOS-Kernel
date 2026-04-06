# HeroOS Architecture

## Overview

HeroOS is a lightweight, developer-centric operating system targeting x86_64
and ARM (Raspberry Pi, mini PCs). It uses a **hybrid microkernel** approach: a
small, fast kernel core handles CPU, memory, scheduling and IPC, while
device drivers and higher-level services can run as separate processes.

---

## Repository Layout

```
HeroOS-Kernel/
├── arch/                  Architecture-specific code
│   ├── x86_64/
│   │   ├── boot/          Multiboot2 + long-mode setup (boot.asm)
│   │   ├── cpu/           GDT, IDT, ISR stubs, context switch
│   │   ├── mm/            x86_64 paging helpers
│   │   └── drivers/       Serial UART, PIT timer, RTC
│   └── arm/
│       ├── boot/          AArch64 EL1 entry (stub)
│       ├── cpu/           AArch64 exception vectors (stub)
│       └── drivers/       RPi4/5 BSP, GIC (stub)
│
├── boot/                  GRUB configuration
├── docs/                  Architecture documents, API reference
├── drivers/               Platform-independent drivers
│   ├── keyboard/          PS/2 keyboard
│   ├── framebuffer/       Linear framebuffer + 8×16 font
│   ├── block/             Block device abstractions (stub)
│   └── net/               Network driver framework (stub)
│
├── fs/                    Filesystem implementations
│   ├── tmpfs/             In-memory filesystem (tmpfs)
│   ├── ext2/              ext2 filesystem (stub)
│   └── fat32/             FAT32 for SD cards (stub)
│
├── include/               Public headers
│   ├── kernel/            Core kernel API (types, printk, hal, driver, vfs)
│   └── lib/               Library headers (string)
│
├── kernel/                Core kernel
│   ├── kmain.c            Entry point
│   ├── mm/                Physical MM (pmm), Virtual MM (vmm), Heap (kheap)
│   ├── proc/              Processes (process), Scheduler (sched)
│   ├── ipc/               Signals, pipes, message queues, shared memory
│   ├── syscall/           System call dispatch table
│   └── fs/                VFS layer
│
├── lib/                   Kernel utility library
│   ├── string.c           memset/memcpy/strcpy/…
│   └── printk.c           printf-style kernel logging
│
├── userspace/             User-space apps and tools
│   ├── init/              PID 1 init system
│   ├── files/             Files — file manager
│   ├── terminal/          Terminal — terminal emulator
│   ├── quill/             Quill — code editor
│   ├── lens/              Lens — web browser
│   ├── prism/             Prism — system settings
│   ├── pulse/             Pulse — system monitor
│   ├── wave/              Wave — media player
│   ├── heroshell/         HeroShell — advanced system shell
│   ├── heropkg/           HeroPkg — package manager
│   └── heroserve/         HeroServe — zero-config HTTP dev server
│
├── ui/                    Display system + desktop environment
│   ├── canvas/            Canvas — compositor (Wayland-inspired)
│   ├── aura/              Aura — desktop environment (tiling + floating WM)
│   ├── orbit/             Orbit — app launcher (Super+Space)
│   └── echo/              Echo — notification system
├── tools/                 linker.ld, build utilities
├── Makefile               Top-level build system
└── .github/workflows/     CI (build + QEMU smoke test)
```

---

## Boot Sequence (x86_64)

```
GRUB/MB2 loader
  └─► arch/x86_64/boot/boot.asm  (_start, 32-bit)
        ├─ Check CPUID / long-mode support
        ├─ Set up initial PML4 page tables
        │    • Identity-map first 2 GiB (0x0 → 0x0)
        │    • Map kernel at higher-half (0xFFFFFFFF80000000 → 0x0)
        ├─ Enable PAE, EFER.LME, CR0.PG
        ├─ Load 64-bit GDT
        └─► _start64 (64-bit)
              ├─ Reload segment registers (0x10 data, 0x08 code)
              ├─ Switch to 64-bit kernel stack
              └─► kernel/kmain.c  (kmain)
                    ├─ 1. serial_early_init  (COM1 115200)
                    ├─ 2. gdt_init           (reload GDT + TSS)
                    ├─ 3. idt_init           (IDT + PIC remap)
                    ├─ 4. pmm_init           (bitmap physical allocator)
                    ├─ 5. vmm_init           (4-level paging)
                    ├─ 6. kheap_init         (first-fit heap)
                    ├─ 7. pit_init / rtc_init
                    ├─ 8. vfs_init + tmpfs_create_root → mount "/"
                    ├─ 9. driver registration + probe
                    ├─10. proc_init + sched_init
                    ├─11. syscall_init
                    ├─12. spawn "init" (PID 1)
                    └─13. sti + hlt loop (idle)
```

---

## Memory Layout (x86_64)

| Virtual Range                    | Purpose                         |
|----------------------------------|---------------------------------|
| `0x0000000000000000–0x7FFFFFFF…` | User space                      |
| `0xFFFF800000000000–…`           | Kernel direct-map region        |
| `0xFFFFFFFF80000000–…`           | Kernel code/data (higher-half)  |
| `0xFFFFFFFF82000000+`            | Kernel heap (64 MiB)            |

Physical:

| Address        | Content                              |
|----------------|--------------------------------------|
| `0x0000–0x00FF`| Real-mode IVT (reserved)             |
| `0x0100–0x7BFF`| BIOS data / free                     |
| `0x7C00–0x7DFF`| Boot sector area                     |
| `0x0100000`    | Kernel ELF loaded here by GRUB       |
| `0x0400000–…`  | Kernel heap physical frames          |

---

## Subsystem Descriptions

### Physical Memory Manager (PMM)
- **Algorithm**: Bitmap allocator (1 bit per 4 KiB page)
- **Source**: `kernel/mm/pmm.c`
- Parses Multiboot2 memory map at boot
- O(n) allocation scan with a free-page hint for amortised O(1)
- Supports contiguous multi-page allocation for DMA buffers

### Virtual Memory Manager (VMM)
- **Algorithm**: 4-level paging (PML4 → PDPT → PD → PT)
- **Source**: `kernel/mm/vmm.c`, `arch/x86_64/mm/paging.c`
- Per-process `addr_space_t` holding the PML4 physical address
- `paging_map` / `paging_unmap` operate on the currently active CR3

### Kernel Heap
- **Algorithm**: First-fit free-list with header+footer blocks
- **Source**: `kernel/mm/kheap.c`
- Supports `kmalloc`, `kcalloc`, `krealloc`, `kfree`
- Coalesces adjacent free blocks on `kfree` to reduce fragmentation
- Expands by allocating new pages from PMM when full

### Scheduler
- **Algorithm**: CFS-inspired round-robin with virtual runtime
- **Source**: `kernel/proc/sched.c`
- Preemptive: PIT IRQ fires at 1 kHz, `sched_tick()` checks quantum
- Voluntary yield via `sched_yield()`, sleep via `sched_sleep(ms)`
- Priority: lower number = higher priority; vruntime weighted by priority

### IPC
- **Signals** (`kernel/ipc/ipc.c`): POSIX-compatible subset (SIGKILL, SIGTERM, SIGSTOP, SIGCONT, …)
- **Pipes**: Circular buffer (4 KiB), `pipe_read` / `pipe_write`
- **Message Queues**: Fixed-size queue (64 msgs × 1 KiB each)
- **Shared Memory**: PMM-backed regions, reference-counted

### VFS
- **Source**: `kernel/fs/vfs.c`
- Filesystem-agnostic layer with `vfs_ops_t` function table
- Path resolution walks the directory tree via `finddir`
- Mount table supports multiple filesystems at different paths
- tmpfs (`fs/tmpfs/tmpfs.c`) provides the root filesystem

### Syscall Interface
- **Vector**: `int 0x80` (Linux-compatible)
- **Source**: `kernel/syscall/syscall.c`
- Dispatch table indexed by `rax`; arguments in `rdi, rsi, rdx, r10, r8, r9`
- Currently implements: exit, getpid, getppid, yield, sleep, kill, read, write, uname

---

## Security Features

| Feature             | Status    | Notes                                      |
|---------------------|-----------|--------------------------------------------|
| Stack canaries      | Planned   | Compiler `-fstack-protector` when userspace is linked |
| NX/XD bit           | Active    | `EFER.NXE` set in boot.asm; PTE_NO_EXEC on data pages |
| KASLR               | Planned   | Randomise kernel load address              |
| SMEP/SMAP           | Planned   | Prevent kernel executing/reading user pages |
| Capabilities        | Planned   | Per-process capability set                 |
| Secure Boot         | Planned   | UEFI Secure Boot signature                 |
| Disk encryption     | Planned   | dm-crypt equivalent in block layer         |

---

## Planned Milestones

| Milestone | Description                                                  |
|-----------|--------------------------------------------------------------|
| 0.1.0     | x86_64 boot, serial output, memory, IDT ✓                    |
| 0.2.0     | Process scheduler, IPC, VFS, tmpfs, syscalls, drivers ✓      |
| 0.3.0     | App suite design (Files, Terminal, Quill, Lens, Prism, …) ✓  |
| 0.4.0     | Full QEMU boot to Terminal + HeroShell prompt                |
| 0.5.0     | TCP/IP stack, HeroServe HTTP server                          |
| 0.6.0     | ext2 filesystem, disk I/O (NVMe/SATA)                        |
| 0.7.0     | AArch64 port (Raspberry Pi 4/5)                             |
| 0.8.0     | Canvas compositor, Aura desktop, Orbit launcher, Echo notifs |
| 0.9.0     | ELF loader, user-space runtime, all apps functional          |
| 1.0.0     | HeroPkg + curated dev tool repository; first public release  |
