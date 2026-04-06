#include "framebuffer.h"
#include <kernel/driver.h>
#include <kernel/printk.h>
#include <lib/string.h>

/* ─── 8×16 bitmap font (embedded minimal font for ASCII 32–127) ──────────── */
/* Each character is 8 columns × 16 rows (16 bytes, each byte = one row). */

#include "font8x16.h"   /* auto-included minimal bitmap font */

/* ─── Framebuffer state ──────────────────────────────────────────────────── */

static fb_info_t g_fb;
static uint32_t *g_fb_mem = NULL;

/* Terminal cursor */
static uint32_t term_col = 0, term_row = 0;
static color_t  term_fg  = COLOR_TEXT;
static color_t  term_bg  = COLOR_BG;
#define FONT_W  8
#define FONT_H  16

/* ─── Pixel operations ───────────────────────────────────────────────────── */

void fb_init(fb_info_t *info) {
    g_fb    = *info;
    g_fb_mem = (uint32_t *)(uintptr_t)info->addr;
    fb_clear(COLOR_BG);
    pr_info("Framebuffer: %ux%u %ubpp @ 0x%llx\n",
            info->width, info->height, info->bpp, (uint64_t)info->addr);
}

void fb_set_pixel(uint32_t x, uint32_t y, color_t color) {
    if (!g_fb_mem || x >= g_fb.width || y >= g_fb.height) return;
    uint32_t offset = y * (g_fb.pitch / 4) + x;
    g_fb_mem[offset] = color;
}

void fb_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, color_t color) {
    for (uint32_t row = y; row < y + h && row < g_fb.height; row++) {
        uint32_t *line = g_fb_mem + row * (g_fb.pitch / 4);
        for (uint32_t col = x; col < x + w && col < g_fb.width; col++)
            line[col] = color;
    }
}

void fb_draw_hline(uint32_t x, uint32_t y, uint32_t len, color_t color) {
    fb_fill_rect(x, y, len, 1, color);
}

void fb_draw_vline(uint32_t x, uint32_t y, uint32_t len, color_t color) {
    for (uint32_t i = 0; i < len; i++) fb_set_pixel(x, y + i, color);
}

void fb_draw_rect_outline(uint32_t x, uint32_t y, uint32_t w, uint32_t h, color_t color) {
    fb_draw_hline(x,         y,         w, color);
    fb_draw_hline(x,         y + h - 1, w, color);
    fb_draw_vline(x,         y,         h, color);
    fb_draw_vline(x + w - 1, y,         h, color);
}

void fb_clear(color_t color) {
    if (!g_fb_mem) return;
    uint32_t total = (g_fb.pitch / 4) * g_fb.height;
    for (uint32_t i = 0; i < total; i++) g_fb_mem[i] = color;
}

void fb_get_info(fb_info_t *out) { *out = g_fb; }

/* ─── Character rendering ────────────────────────────────────────────────── */

void fb_draw_char(uint32_t x, uint32_t y, char c, color_t fg, color_t bg) {
    if (!g_fb_mem) return;
    uint8_t idx = (uint8_t)c < 32 ? '?' - 32 : (uint8_t)c - 32;
    if (idx >= 96) idx = 0;
    const uint8_t *glyph = font8x16 + idx * FONT_H;

    for (uint32_t row = 0; row < FONT_H; row++) {
        uint8_t bits = glyph[row];
        for (uint32_t col = 0; col < FONT_W; col++) {
            bool set = (bits >> (7 - col)) & 1;
            fb_set_pixel(x + col, y + row, set ? fg : bg);
        }
    }
}

void fb_draw_string(uint32_t x, uint32_t y, const char *s, color_t fg, color_t bg) {
    uint32_t cx = x;
    while (*s) {
        if (*s == '\n') { cx = x; y += FONT_H; }
        else { fb_draw_char(cx, y, *s, fg, bg); cx += FONT_W; }
        s++;
    }
}

/* ─── Framebuffer terminal ───────────────────────────────────────────────── */

void fb_scroll_up(uint32_t lines) {
    uint32_t bytes_per_row = g_fb.pitch;
    uint32_t scroll_pixels = lines * FONT_H;
    uint32_t keep_bytes = (g_fb.height - scroll_pixels) * bytes_per_row;

    memmove(g_fb_mem,
            (uint8_t *)g_fb_mem + scroll_pixels * bytes_per_row,
            keep_bytes);
    fb_fill_rect(0, g_fb.height - scroll_pixels, g_fb.width, scroll_pixels, term_bg);
}

void fb_term_init(void) {
    term_col = term_row = 0;
    term_fg  = COLOR_TEXT;
    term_bg  = COLOR_BG;
}

void fb_term_set_color(color_t fg, color_t bg) {
    term_fg = fg;
    term_bg = bg;
}

void fb_term_putc(char c) {
    if (!g_fb_mem) return;
    uint32_t cols = g_fb.width  / FONT_W;
    uint32_t rows = g_fb.height / FONT_H;

    if (c == '\n') {
        term_col = 0;
        term_row++;
    } else if (c == '\r') {
        term_col = 0;
    } else if (c == '\b') {
        if (term_col > 0) {
            term_col--;
            fb_draw_char(term_col * FONT_W, term_row * FONT_H, ' ', term_fg, term_bg);
        }
    } else {
        fb_draw_char(term_col * FONT_W, term_row * FONT_H, c, term_fg, term_bg);
        term_col++;
        if (term_col >= cols) { term_col = 0; term_row++; }
    }

    if (term_row >= rows) {
        fb_scroll_up(1);
        term_row = rows - 1;
    }
}

void fb_term_puts(const char *s) {
    while (*s) fb_term_putc(*s++);
}

/* ─── Driver ─────────────────────────────────────────────────────────────── */

static int fb_probe(driver_t *drv) {
    (void)drv;
    if (!g_fb_mem) return -1;
    fb_term_init();
    return 0;
}

static driver_t fb_driver = {
    .name  = "framebuffer",
    .type  = DRIVER_DISPLAY,
    .state = DRIVER_UNLOADED,
    .probe = fb_probe,
};

void framebuffer_driver_register(void) {
    driver_register(&fb_driver);
}
