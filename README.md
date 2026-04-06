# HeroOS — One OS, Many Platforms

> **"Code without limits. Build without walls."**

HeroOS is a lightweight, developer-centric operating system built from scratch,
targeting **x86_64** and **ARM/AArch64** (Raspberry Pi, mini PCs).

[![CI](https://github.com/PrathameshBarbole/HeroOS-Kernel/actions/workflows/ci.yml/badge.svg)](https://github.com/PrathameshBarbole/HeroOS-Kernel/actions/workflows/ci.yml)

---

## ✨ Philosophy

| Goal | Description |
|------|-------------|
| **Developer-first** | Git, Python, Node/Bun, Go, Rust ship out of the box |
| **Lightweight** | <200 MB base ISO; <20 MB kernel RAM at idle |
| **Fast** | Sub-5 s boot; preemptive CFS-inspired scheduler |
| **Secure** | NX/XD, KASLR, capabilities, sandboxing |
| **Modern UI** | Dark-themed compositor with tiling + floating WM |
| **Multi-platform** | x86_64 laptops/desktops + RPi 4/5 + mini PCs |

---

## 🗂 Repository Structure

```
HeroOS-Kernel/
├── arch/            Architecture-specific (x86_64, arm)
│   ├── x86_64/
│   │   ├── boot/    Multiboot2 + long-mode entry (boot.asm)
│   │   ├── cpu/     GDT, IDT, ISR, context switch (.asm + .c)
│   │   ├── mm/      x86_64 paging (paging.c)
│   │   └── drivers/ Serial UART, PIT timer, RTC
│   └── arm/         AArch64 stubs (RPi 4/5 BSP)
├── boot/            GRUB configuration
├── docs/            Architecture docs, API reference
├── drivers/         Platform-independent drivers
│   ├── keyboard/    PS/2 keyboard
│   ├── framebuffer/ Linear framebuffer + 8×16 bitmap font
│   ├── block/       Block device framework (stub)
│   └── net/         Network driver framework (stub)
├── fs/              Filesystem implementations
│   ├── tmpfs/       In-memory filesystem
│   ├── ext2/        ext2 (stub)
│   └── fat32/       FAT32 for SD cards (stub)
├── include/         Public kernel headers
│   ├── kernel/      types, printk, hal, driver, vfs
│   └── lib/         string.h
├── kernel/          Core kernel
│   ├── kmain.c      Boot entry point
│   ├── mm/          PMM, VMM, kernel heap
│   ├── proc/        Processes, CFS scheduler
│   ├── ipc/         Signals, pipes, msgq, shm
│   ├── syscall/     Linux-compatible syscall table
│   └── fs/          VFS layer
├── lib/             Freestanding string + printk
├── userspace/       User-space components
│   ├── init/        PID 1 init system
│   ├── heroshell/   HeroShell interactive shell
│   ├── heropkg/     HeroPkg package manager
│   └── heroserve/   Zero-config HTTP dev server
├── ui/              Compositor + desktop env (Phase 5)
├── tools/           linker.ld, build scripts
├── Makefile         Top-level build
└── .github/         CI workflows
```

---

## 🚀 Quick Start

### Prerequisites

```bash
# Ubuntu / Debian
sudo apt-get install \
  gcc binutils nasm \
  gcc-x86-64-linux-gnu binutils-x86-64-linux-gnu \
  xorriso grub-pc-bin grub-common \
  qemu-system-x86 make

# macOS (Homebrew with cross-compiler)
brew install x86_64-elf-gcc x86_64-elf-binutils nasm xorriso qemu
```

### Build & Run in QEMU

```bash
# Build kernel ELF
make CROSS_PREFIX="x86_64-linux-gnu-"

# Build bootable ISO
make CROSS_PREFIX="x86_64-linux-gnu-" iso

# Run in QEMU (serial console)
make qemu

# Run with KVM acceleration
make qemu-kvm

# Run with graphical display
make qemu-gui
```

### Expected serial output

```
[HeroOS] Bootloader handoff received. Starting kernel...

[INFO]   GDT initialised
[INFO]   IDT initialised (256 entries, PIC remapped to 0x20-0x2f)
[INFO]   PMM: 255 MiB total, 252 MiB free
[INFO]   VMM initialised
[INFO]   Heap initialised: base=0x2000000, initial=64 KiB
[INFO]   PIT initialised: 1000 Hz (divisor 1193)
[INFO]   VFS initialised
[INFO]   tmpfs: mounted in-memory filesystem
...

  ██╗  ██╗███████╗██████╗  ██████╗  ██████╗ ███████╗
  ...
  HeroOS v0.1.0 — One OS, Many Platforms
```

---

## 🏗 Architecture

### Boot Sequence (x86_64)

```
GRUB (Multiboot2)
  └→ boot.asm (_start, 32-bit)
       ├ Verify CPUID / long mode
       ├ Build PML4 page tables (identity + higher-half)
       ├ Enable PAE, EFER.LME, CR0.PG
       ├ Load 64-bit GDT, far-jump to 64-bit
       └→ kmain(mb2_info_phys)
            ├ 1. UART serial init
            ├ 2. GDT reload + TSS
            ├ 3. IDT + PIC remap (0x20–0x2F)
            ├ 4. Physical memory manager (bitmap)
            ├ 5. Virtual memory (4-level paging)
            ├ 6. Kernel heap (first-fit free-list)
            ├ 7. PIT 1 kHz + RTC
            ├ 8. VFS + tmpfs root mount
            ├ 9. Driver framework (keyboard, framebuffer)
            ├10. Process manager + CFS scheduler
            ├11. Syscall interface (int 0x80)
            ├12. Print banner
            └13. Spawn init (PID 1) → sti + hlt
```

### Memory Map (x86_64 virtual)

| Range | Purpose |
|-------|---------|
| `0x0000_0000_0000_0000 – 0x0000_7FFF_FFFF_FFFF` | User space |
| `0xFFFF_8000_0000_0000 – …` | Kernel direct-map |
| `0xFFFF_FFFF_8000_0000 – …` | Kernel code/data (higher-half) |
| `0xFFFF_FFFF_8200_0000 – …` | Kernel heap (64 MiB) |

---

## 🛠 Developer Tools (via HeroPkg)

```bash
heropkg install bun       # Bun JS runtime
heropkg install node      # Node.js
heropkg install python    # Python 3.12
heropkg install go        # Go 1.22
heropkg install rust      # Rust
heropkg install git       # Git
heropkg install cmake     # CMake
heropkg install micro     # Terminal editor
```

---

## 🌐 HeroServe — Zero-Config Dev Server

```bash
hero serve          # Serve CWD on :8080
hero serve 3000     # Serve on :3000
hero serve ./dist   # Serve ./dist directory
```

No config files. No login. Instant start.

---

## 📋 Roadmap

| Milestone | Status | Description |
|-----------|--------|-------------|
| **0.1** | ✅ | x86_64 boot, serial, IDT, PMM, VMM, heap, PIT |
| **0.2** | ✅ | Process scheduler, IPC, VFS, tmpfs, syscalls |
| **0.3** | 🔄 | Full QEMU boot to HeroShell prompt |
| **0.4** | 📋 | TCP/IP stack, HeroServe HTTP server |
| **0.5** | 📋 | ext2 filesystem, NVMe/SATA drivers |
| **0.6** | 📋 | AArch64 port (Raspberry Pi 4/5) |
| **0.7** | 📋 | Framebuffer compositor, HeroDE window manager |
| **0.8** | 📋 | ELF loader, user-space runtime |
| **0.9** | 📋 | HeroPkg + curated dev tool repository |
| **1.0** | 📋 | First public release |

---

## 🔒 Security

- **NX/XD bit**: `EFER.NXE` set at boot; data pages marked non-executable
- **Double-fault stack (IST)**: Separate stack for double-fault handler
- **KASLR** *(planned)*: Randomise kernel physical load address
- **SMEP/SMAP** *(planned)*: Prevent kernel from executing/accessing user memory
- **Capabilities** *(planned)*: Fine-grained per-process permissions
- **Sandboxing** *(planned)*: Namespace-based process isolation
- **Disk encryption** *(planned)*: Full-disk encryption in block layer

---

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch: `git checkout -b feature/my-feature`
3. Build and test: `make && make qemu`
4. Open a pull request

See [docs/architecture.md](docs/architecture.md) for the full design document.

---

## 📄 License

[MIT License](LICENSE) — © 2024 Prathamesh Barbole