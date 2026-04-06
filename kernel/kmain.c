/*
 * HeroOS — Kernel Entry Point
 *
 * kmain() is called from arch/x86_64/boot/boot.asm after the CPU has entered
 * 64-bit long mode.  The argument mb2_info is the physical address of the
 * Multiboot2 information structure passed by GRUB (or any MB2-compliant loader).
 *
 * Boot sequence:
 *   1. Serial UART early output (allows debugging before everything else)
 *   2. GDT reload (64-bit descriptors + TSS)
 *   3. IDT + PIC setup (exception/IRQ handlers)
 *   4. Physical memory manager (parses MB2 memory map)
 *   5. Virtual memory manager + paging
 *   6. Kernel heap
 *   7. PIT timer (1000 Hz)
 *   8. VFS + tmpfs
 *   9. Driver framework + keyboard + framebuffer
 *  10. Process manager + scheduler
 *  11. Syscall interface
 *  12. Display HeroOS boot banner
 *  13. Spawn init process
 *  14. Enable interrupts and hand off to scheduler
 */

#include <kernel/types.h>
#include <kernel/printk.h>
#include <kernel/mm/pmm.h>
#include <kernel/mm/vmm.h>
#include <kernel/mm/kheap.h>
#include <kernel/proc/process.h>
#include <kernel/proc/sched.h>
#include <kernel/syscall/syscall.h>
#include <kernel/fs/vfs.h>
#include <kernel/driver.h>
#include <lib/string.h>

#include <arch/x86_64/cpu/gdt.h>
#include <arch/x86_64/cpu/idt.h>
#include <arch/x86_64/drivers/serial.h>
#include <arch/x86_64/drivers/pit.h>

#include <drivers/keyboard/keyboard.h>
#include <drivers/framebuffer/framebuffer.h>
#include <fs/tmpfs/tmpfs.h>

/* ─── Multiboot2 framebuffer tag ─────────────────────────────────────────── */

#define MB2_TAG_FRAMEBUFFER  8

typedef struct PACKED {
    uint32_t  type;
    uint32_t  size;
    uint64_t  framebuffer_addr;
    uint32_t  framebuffer_pitch;
    uint32_t  framebuffer_width;
    uint32_t  framebuffer_height;
    uint8_t   framebuffer_bpp;
    uint8_t   framebuffer_type;
    uint16_t  reserved;
} mb2_fb_tag_t;

/* ─── Banner ──────────────────────────────────────────────────────────────── */

static void print_banner(void) {
    printk("\n");
    printk("  ██╗  ██╗███████╗██████╗  ██████╗  ██████╗ ███████╗\n");
    printk("  ██║  ██║██╔════╝██╔══██╗██╔═══██╗██╔═══██╗██╔════╝\n");
    printk("  ███████║█████╗  ██████╔╝██║   ██║██║   ██║███████╗\n");
    printk("  ██╔══██║██╔══╝  ██╔══██╗██║   ██║██║   ██║╚════██║\n");
    printk("  ██║  ██║███████╗██║  ██║╚██████╔╝╚██████╔╝███████║\n");
    printk("  ╚═╝  ╚═╝╚══════╝╚═╝  ╚═╝ ╚═════╝  ╚═════╝ ╚══════╝\n");
    printk("\n");
    printk("  HeroOS v0.1.0 — One OS, Many Platforms\n");
    printk("  Developer-Centric | Lightweight | Blazing Fast\n");
    printk("  x86_64 | ARM/AArch64 | RISC-V64\n");
    printk("\n");
    printk("  \"Code without limits. Build without walls.\"\n");
    printk("\n");
    printk("─────────────────────────────────────────────────────\n");
}

/* ─── Find MB2 tag ────────────────────────────────────────────────────────── */

static void *mb2_find_tag(uintptr_t mb2_phys, uint32_t tag_type) {
    if (!mb2_phys) return NULL;
    mb2_info_t *info = (mb2_info_t *)mb2_phys;
    uint8_t *p   = (uint8_t *)mb2_phys + 8;
    uint8_t *end = (uint8_t *)mb2_phys + info->total_size;

    while (p < end) {
        mb2_tag_t *tag = (mb2_tag_t *)p;
        if (tag->type == 0) break;
        if (tag->type == tag_type) return tag;
        p += ALIGN_UP(tag->size, 8);
    }
    return NULL;
}

/* ─── Init process ────────────────────────────────────────────────────────── */

static void init_process(void) {
    pr_info("init: HeroOS userspace init started (PID 1)\n");

    /* Create basic directory structure */
    vfs_mkdir("/tmp",  0777);
    vfs_mkdir("/dev",  0755);
    vfs_mkdir("/bin",  0755);
    vfs_mkdir("/proc", 0555);
    vfs_mkdir("/home", 0755);
    vfs_mkdir("/etc",  0755);
    vfs_mkdir("/var",  0755);

    /* Write a motd to /etc/motd */
    vfs_node_t *motd = vfs_open("/etc/motd", O_CREAT | O_WRONLY);
    if (motd) {
        const char *msg = "Welcome to HeroOS — One OS, Many Platforms.\n"
                          "Type 'hero help' to get started.\n";
        vfs_write(motd, 0, strlen(msg), msg);
        vfs_close(motd);
    }

    pr_info("init: filesystem layout created\n");
    pr_info("init: HeroOS is ready.\n\n");

    printk("┌──────────────────────────────────────────────────┐\n");
    printk("│  HeroOS is ready!                                │\n");
    printk("│                                                  │\n");
    printk("│  Available commands (future heroshell):          │\n");
    printk("│    hero serve   — start HeroServe HTTP server    │\n");
    printk("│    hero pkg     — HeroPkg package manager        │\n");
    printk("│    hero info    — system information             │\n");
    printk("│                                                  │\n");
    printk("│  Developer tools: bun, npm, git, python, go      │\n");
    printk("│  Install via: heropkg install <tool>             │\n");
    printk("└──────────────────────────────────────────────────┘\n\n");

    /* Idle — in a future release this will exec /bin/heroshell */
    for (;;) sched_yield();
}

/* ─── kmain ───────────────────────────────────────────────────────────────── */

void kmain(uintptr_t mb2_info_phys) {
    /* Step 1: Early serial output (so we can print before FB is up) */
    serial_early_init();
    printk("\n[HeroOS] Bootloader handoff received. Starting kernel...\n\n");

    /* Step 2: GDT */
    gdt_init();
    pr_info("GDT initialised\n");

    /* Step 3: IDT + PIC */
    idt_init();

    /* Step 4: Physical memory manager */
    pmm_init(mb2_info_phys);

    /* Step 5: Virtual memory manager */
    vmm_init();

    /* Step 6: Kernel heap (starts at 32 MiB physical, 64 MiB size) */
    kheap_init(0x02000000UL, 64 * 1024 * 1024);

    /* Step 7: PIT timer at 1000 Hz */
    pit_init(1000);
    rtc_init();

    /* Step 8: VFS + tmpfs */
    vfs_init();
    tmpfs_register();
    vfs_node_t *rootfs = tmpfs_create_root();
    if (rootfs) {
        vfs_mount("/", rootfs);
    } else {
        kernel_panic("Failed to create root filesystem");
    }

    /* Step 9: Driver framework + devices */
    keyboard_driver_register();
    framebuffer_driver_register();

    /* Check for framebuffer from MB2 */
    mb2_fb_tag_t *fb_tag = (mb2_fb_tag_t *)mb2_find_tag(mb2_info_phys, MB2_TAG_FRAMEBUFFER);
    if (fb_tag && fb_tag->framebuffer_addr) {
        fb_info_t fb_info = {
            .addr   = (uintptr_t)fb_tag->framebuffer_addr,
            .pitch  = fb_tag->framebuffer_pitch,
            .width  = fb_tag->framebuffer_width,
            .height = fb_tag->framebuffer_height,
            .bpp    = fb_tag->framebuffer_bpp,
            .type   = fb_tag->framebuffer_type,
        };
        fb_init(&fb_info);
        fb_term_init();
    }

    drivers_probe_all();

    /* Step 10: Process manager + scheduler */
    proc_init();
    sched_init();

    /* Step 11: Syscall interface */
    syscall_init();

    /* Print the banner */
    print_banner();

    /* Memory report */
    pmm_stats_t stats;
    pmm_get_stats(&stats);
    pr_info("Memory: %llu MiB total, %llu MiB free, %llu MiB used\n",
            (stats.total_pages * PAGE_SIZE) >> 20,
            (stats.free_pages  * PAGE_SIZE) >> 20,
            (stats.used_pages  * PAGE_SIZE) >> 20);

    /* Step 12: Spawn init (PID 1) */
    process_t *init = proc_create_kernel_thread("init", init_process);
    if (!init) kernel_panic("Failed to create init process");
    sched_add(init);

    /* Step 13: Enable interrupts and start the scheduler */
    pr_info("Entering scheduler. Welcome to HeroOS.\n\n");
    irq_enable();

    /* Hand off to idle loop — scheduler takes over via PIT IRQ */
    for (;;) {
        __asm__ volatile("hlt");
    }
}
