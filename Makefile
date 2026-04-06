# ─────────────────────────────────────────────────────────────────────────────
# HeroOS Top-level Makefile
#
# Usage:
#   make              — Build kernel ELF + ISO
#   make iso          — Build bootable ISO image
#   make qemu         — Run in QEMU (no display)
#   make qemu-kvm     — Run in QEMU with KVM acceleration
#   make clean        — Remove build artefacts
#   make docs         — Generate HTML documentation (requires Doxygen)
#
# Cross-compilers required:
#   x86_64-elf-gcc, x86_64-elf-ld, nasm
#
# Install on Ubuntu/Debian:
#   sudo apt-get install gcc-x86-64-linux-gnu binutils-x86-64-linux-gnu nasm
#   (or build a bare-metal cross-compiler with crosstool-ng)
# ─────────────────────────────────────────────────────────────────────────────

# ─── Toolchain ────────────────────────────────────────────────────────────────
# Support both a bare-metal cross-compiler (x86_64-elf-*) and a standard
# x86_64 Linux GCC (used with -ffreestanding + no standard libraries).
CROSS_PREFIX ?= x86_64-elf-
CC   := $(CROSS_PREFIX)gcc
LD   := $(CROSS_PREFIX)ld
AS   := nasm
GRUB := grub-mkrescue

# Fall back to host GCC if cross-compiler not found
ifeq ($(shell which $(CC) 2>/dev/null),)
  CC := gcc
  LD := ld
endif

# ─── Directories ─────────────────────────────────────────────────────────────
BUILD_DIR  := build
ISO_DIR    := $(BUILD_DIR)/iso
KERNEL_ELF := $(BUILD_DIR)/herokernel.elf
KERNEL_ISO := $(BUILD_DIR)/HeroOS.iso

# ─── Flags ───────────────────────────────────────────────────────────────────
CFLAGS := \
    -std=c11 \
    -ffreestanding \
    -fno-stack-protector \
    -fno-builtin \
    -fno-pic \
    -mno-red-zone \
    -mno-mmx \
    -mno-sse \
    -mno-sse2 \
    -mcmodel=kernel \
    -Wall -Wextra \
    -Wno-unused-parameter \
    -O2 \
    -g \
    -I. \
    -Iinclude \
    -Iarch/x86_64 \
    -Idrivers \
    -Ifs \
    -Iuserspace

ASFLAGS := -f elf64

LDFLAGS := \
    -T tools/linker.ld \
    -nostdlib \
    -z max-page-size=0x1000 \
    -Map $(BUILD_DIR)/kernel.map

# ─── Source files ─────────────────────────────────────────────────────────────

# Assembly
ASM_SRCS := \
    arch/x86_64/boot/boot.asm \
    arch/x86_64/cpu/gdt.asm \
    arch/x86_64/cpu/idt.asm \
    arch/x86_64/cpu/isr.asm \
    arch/x86_64/cpu/switch.asm

# C kernel sources
C_SRCS := \
    kernel/kmain.c \
    kernel/mm/pmm.c \
    kernel/mm/vmm.c \
    kernel/mm/kheap.c \
    kernel/proc/process.c \
    kernel/proc/sched.c \
    kernel/ipc/ipc.c \
    kernel/syscall/syscall.c \
    kernel/fs/vfs.c \
    arch/x86_64/cpu/gdt.c \
    arch/x86_64/cpu/idt.c \
    arch/x86_64/drivers/serial.c \
    arch/x86_64/drivers/pit.c \
    drivers/driver.c \
    drivers/keyboard/keyboard.c \
    drivers/framebuffer/framebuffer.c \
    fs/tmpfs/tmpfs.c \
    lib/string.c \
    lib/printk.c \
    userspace/init/init.c \
    userspace/heroshell/heroshell.c

# Object files
ASM_OBJS := $(patsubst %.asm, $(BUILD_DIR)/%.o, $(ASM_SRCS))
C_OBJS   := $(patsubst %.c,   $(BUILD_DIR)/%.o, $(C_SRCS))
ALL_OBJS := $(ASM_OBJS) $(C_OBJS)

# ─── Default target ───────────────────────────────────────────────────────────
.PHONY: all iso qemu qemu-kvm clean docs

all: $(KERNEL_ELF)

# ─── Link ─────────────────────────────────────────────────────────────────────
$(KERNEL_ELF): $(ALL_OBJS) tools/linker.ld
	@mkdir -p $(BUILD_DIR)
	@echo "  LD    $@"
	$(LD) $(LDFLAGS) -o $@ $(ALL_OBJS)
	@echo ""
	@echo "Kernel ELF: $@"
	@size $@

# ─── Compile C ────────────────────────────────────────────────────────────────
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "  CC    $<"
	$(CC) $(CFLAGS) -c $< -o $@

# ─── Assemble ─────────────────────────────────────────────────────────────────
$(BUILD_DIR)/%.o: %.asm
	@mkdir -p $(dir $@)
	@echo "  AS    $<"
	$(AS) $(ASFLAGS) $< -o $@

# ─── ISO image ────────────────────────────────────────────────────────────────
iso: $(KERNEL_ELF)
	@mkdir -p $(ISO_DIR)/boot/grub
	cp $(KERNEL_ELF) $(ISO_DIR)/boot/herokernel.elf
	cp boot/grub.cfg $(ISO_DIR)/boot/grub/grub.cfg
	$(GRUB) -o $(KERNEL_ISO) $(ISO_DIR)
	@echo ""
	@echo "ISO image: $(KERNEL_ISO)"

# ─── QEMU (serial console) ────────────────────────────────────────────────────
QEMU_FLAGS := \
    -machine q35 \
    -cpu qemu64 \
    -m 256M \
    -serial stdio \
    -display none \
    -no-reboot \
    -no-shutdown

qemu: $(KERNEL_ISO)
	qemu-system-x86_64 $(QEMU_FLAGS) -cdrom $(KERNEL_ISO)

qemu-kvm: $(KERNEL_ISO)
	qemu-system-x86_64 $(QEMU_FLAGS) -enable-kvm -cdrom $(KERNEL_ISO)

# QEMU without ISO (load kernel directly via multiboot)
qemu-kernel: $(KERNEL_ELF)
	qemu-system-x86_64 $(QEMU_FLAGS) \
	    -kernel $(KERNEL_ELF) \
	    -append "heroOS boot"

# QEMU with graphical display
qemu-gui: $(KERNEL_ISO)
	qemu-system-x86_64 \
	    -machine q35 \
	    -cpu qemu64 \
	    -m 256M \
	    -serial stdio \
	    -vga virtio \
	    -no-reboot \
	    -cdrom $(KERNEL_ISO)

# ─── Clean ────────────────────────────────────────────────────────────────────
clean:
	rm -rf $(BUILD_DIR)
	@echo "Build directory removed."

# ─── Docs (Doxygen) ──────────────────────────────────────────────────────────
docs:
	doxygen docs/Doxyfile

# ─── Dependency tracking ──────────────────────────────────────────────────────
-include $(ALL_OBJS:.o=.d)
$(BUILD_DIR)/%.d: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MM -MT '$(BUILD_DIR)/$*.o' $< > $@
