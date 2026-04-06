#include "keyboard.h"
#include <arch/x86_64/cpu/idt.h>
#include <kernel/driver.h>
#include <kernel/printk.h>

/* ─── US QWERTY scancode-to-ASCII maps ───────────────────────────────────── */

static const char scancode_normal[128] = {
    0,   0x1B,'1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t','q',  'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0,   'a',  's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'','`',
    0,   '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0,   ' ', 0,
};

static const char scancode_shift[128] = {
    0,   0x1B,'!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t','Q',  'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0,   'A',  'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0,   '|',  'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0,   ' ', 0,
};

/* ─── Keyboard state ─────────────────────────────────────────────────────── */

static key_event_t kbd_buffer[KBD_BUFFER_SIZE];
static uint32_t    kbd_head = 0;
static uint32_t    kbd_tail = 0;

static bool shift_pressed    = false;
static bool ctrl_pressed     = false;
static bool alt_pressed      = false;
static bool caps_lock        = false;
static bool extended_pending = false;  /* received 0xE0 prefix */

/* ─── Port I/O ───────────────────────────────────────────────────────────── */

static inline uint8_t inb(uint16_t port) {
    uint8_t v; __asm__ volatile("inb %1, %0" : "=a"(v) : "Nd"(port)); return v;
}

/* ─── IRQ handler ────────────────────────────────────────────────────────── */

static void keyboard_irq_handler(interrupt_frame_t *frame) {
    (void)frame;
    uint8_t scancode = inb(KBD_DATA_PORT);

    /* 0xE0 is a prefix byte for extended keys (e.g. arrow keys, Home, End).
     * The actual scancode arrives in the next interrupt. Remember the prefix
     * and wait for that second byte before generating a key event. */
    if (scancode == 0xE0) {
        extended_pending = true;
        return;
    }

    bool extended = extended_pending;
    extended_pending = false;

    bool released = (scancode & 0x80) != 0;
    scancode &= 0x7F;

    /* Update modifier state (only for non-extended modifier keys) */
    if (!extended) {
        if (scancode == KEY_LSHIFT || scancode == KEY_RSHIFT) {
            shift_pressed = !released;
            return;
        }
        if (scancode == KEY_CTRL)  { ctrl_pressed  = !released; return; }
        if (scancode == KEY_ALT)   { alt_pressed   = !released; return; }
        if (scancode == KEY_CAPS && !released) { caps_lock = !caps_lock; return; }
    }

    if (released) return;   /* Only handle key-down for character events */

    /* Derive ASCII for normal (non-extended) keys */
    bool use_shift = shift_pressed ^ caps_lock;
    char ascii = 0;
    if (!extended && scancode < 128)
        ascii = use_shift ? scancode_shift[scancode] : scancode_normal[scancode];

    key_event_t event = {
        .scancode = scancode,
        .ascii    = ascii,
        .pressed  = !released,
        .shift    = shift_pressed,
        .ctrl     = ctrl_pressed,
        .alt      = alt_pressed,
        .extended = extended,
    };

    /* Buffer the event */
    uint32_t next_tail = (kbd_tail + 1) % KBD_BUFFER_SIZE;
    if (next_tail != kbd_head) {
        kbd_buffer[kbd_tail] = event;
        kbd_tail = next_tail;
    }
}

/* ─── Public API ─────────────────────────────────────────────────────────── */

static int keyboard_probe(driver_t *drv) {
    (void)drv;
    irq_register_handler(1, keyboard_irq_handler);
    pr_info("PS/2 keyboard driver ready\n");
    return 0;
}

void keyboard_init(void) {
    keyboard_driver_register();
}

bool keyboard_poll(key_event_t *event) {
    if (kbd_head == kbd_tail) return false;
    *event   = kbd_buffer[kbd_head];
    kbd_head = (kbd_head + 1) % KBD_BUFFER_SIZE;
    return true;
}

char keyboard_getchar(void) {
    key_event_t ev;
    while (!keyboard_poll(&ev) || !ev.ascii || ev.extended)
        __asm__ volatile("hlt");
    return ev.ascii;
}

/* ─── Driver registration ────────────────────────────────────────────────── */

static driver_t keyboard_driver = {
    .name  = "ps2-keyboard",
    .type  = DRIVER_INPUT,
    .state = DRIVER_UNLOADED,
    .probe = keyboard_probe,
};

void keyboard_driver_register(void) {
    driver_register(&keyboard_driver);
}
